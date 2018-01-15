using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public static class Protocol
{

        public enum reqType
        {
            req_cnt,
            req_snd,
            req_rcv,
            req_res
        };

        /* Defines connection types.                                    */
        public enum connType
        {
            conn_client,
            conn_slave
        };

        /* Defines file type.                                          */
        public enum fileType
        {
            file_py2_script,
            file_py3_script,
            file_data_file
        };

        /*Defines server response types                                */
        public enum resType
        {
            res_ok,
            res_fail,
            res_full,
            res_unauth_dev,
            res_empty
        };

        /* Defines request header.                                      */
       public struct Header
        {
           public reqType req_type;
           public ulong size;
           public string key;
        };

    /* Defines request.                                             */
    public struct Request
        {
            public Header header;
            public IntPtr data;
        };

    /* Defines login request data.                                   */
    public struct RData_Connect
        {
            public connType conn_type;
            public string name;
            public string password;
        };

    /* Defines file sending request data.                            */
    public struct RData_File
        {
            public int id;
            public fileType file_type;
            public ulong size;
            public string data;
        };

    /* Defines server response                                       */
    public struct RData_Response
        {
            public resType res_type;
            public int id;
        };

   
    public static int HEADER_KEY_LENGTH = 9;
    public static int NICKNAME_LENGTH = 17;
    public static int PASSWORD_LENGTH = 17;

    public static void req_decode(ref Request request, byte[] data, bool header_only = false)
    {
        /* header fields*/
        
            int counter = 0;

            byte[] htype = new byte[sizeof(int)];
            Array.Copy(data, counter, htype, 0, sizeof(int));
            request.header.req_type = (reqType)BitConverter.ToInt32(htype, 0);
            counter += sizeof(int);

            byte[] hsize = new byte[sizeof(ulong)];
            Array.Copy(data, counter, hsize, 0, sizeof(ulong));
            request.header.size = BitConverter.ToUInt64(hsize, 0);
            counter += sizeof(ulong);

            byte[] hkey = new byte[HEADER_KEY_LENGTH];
            Array.Copy(data, counter, hkey, 0, HEADER_KEY_LENGTH);
            request.header.key = Convert.ToBase64String(hkey);
            counter += HEADER_KEY_LENGTH;

        if (header_only == true)
        {
            return;
        }
        
            
            /* data fields */
            if (request.header.req_type == reqType.req_cnt)
            {
                RData_Connect dt = new RData_Connect();

                byte[] rtype = new byte[sizeof(int)];
                Array.Copy(data, counter, rtype, 0, sizeof(int));
                dt.conn_type = (connType)BitConverter.ToUInt32(rtype, 0);
                counter += sizeof(int);

                byte[] rname = new byte[NICKNAME_LENGTH];
                Array.Copy(data, counter, rname, 0, NICKNAME_LENGTH);
                dt.name = Convert.ToBase64String(rname);
                counter += NICKNAME_LENGTH;

                byte[] rpsswd = new byte[PASSWORD_LENGTH];
                Array.Copy(data, counter, rpsswd, 0, PASSWORD_LENGTH);
                dt.password = Convert.ToBase64String(rpsswd);
                counter += PASSWORD_LENGTH;


                request.data = Marshal.AllocHGlobal(Marshal.SizeOf(dt));
                Marshal.StructureToPtr(dt, request.data, false);
            }

            else if (request.header.req_type == reqType.req_snd)
            {
                RData_File dt = new RData_File();

                byte[] rid = new byte[sizeof(int)];
                Array.Copy(data, counter, rid, 0, sizeof(int));
                dt.id = BitConverter.ToInt32(rid, 0);
                counter += sizeof(int);

                byte[] rtype = new byte[sizeof(int)];
                Array.Copy(data, counter, rtype, 0, sizeof(int));
                dt.file_type = (fileType)BitConverter.ToInt32(rid, 0);
                counter += sizeof(int);

                byte[] rsize = new byte[sizeof(ulong)];
                Array.Copy(data, counter, rsize, 0, sizeof(ulong));
                dt.size = BitConverter.ToUInt64(rsize, 0);
                counter += sizeof(ulong);

                byte[] rdata = new byte[dt.size];
                Array.Copy(data, counter, rdata, 0, (int)dt.size);
                counter += (int)dt.size;

                request.data = Marshal.AllocHGlobal(Marshal.SizeOf(dt));
                Marshal.StructureToPtr(dt, request.data, false);
            }

            else if (request.header.req_type == reqType.req_res)
            {
                RData_Response dt;
                byte[] rtype = new byte[sizeof(int)];
                Array.Copy(data, counter, rtype, 0, sizeof(int));
                dt.res_type = (resType)BitConverter.ToInt32(rtype, 0);
                counter += sizeof(int);

                byte[] rid = new byte[sizeof(int)];
                Array.Copy(data, counter, rid, 0, sizeof(int));
                dt.id = BitConverter.ToInt32(rid, 0);
                counter += sizeof(int);

                request.data = Marshal.AllocHGlobal(Marshal.SizeOf(dt));
                Marshal.StructureToPtr(dt, request.data, false);
            }
        

    }

    static byte[] StringToByteArray(string str, int length)
    {
        return Encoding.ASCII.GetBytes(str.PadRight(length, '\0'));
    }

    public static byte[] combine(byte[] array1, byte[] array2, int size)
    {
        byte[] rv = new byte[array1.Length + array2.Length];
        System.Buffer.BlockCopy(array1, 0, rv, 0, array1.Length);
        System.Buffer.BlockCopy(array2, 0, rv, array1.Length, size);
        return rv;
    }

    public static byte[] req_encode(ref Request request) {
        /* encode header */
        byte[] htype = BitConverter.GetBytes((int)request.header.req_type);
        byte[] hsize = BitConverter.GetBytes(request.header.size);
        byte[] hkey = StringToByteArray(request.header.key, Protocol.HEADER_KEY_LENGTH);

        int oft = htype.Length + hsize.Length + HEADER_KEY_LENGTH;
        byte[] header = new byte[oft];
        System.Buffer.BlockCopy(htype, 0, header, 0, htype.Length);
        System.Buffer.BlockCopy(hkey, 0, header, htype.Length + hsize.Length, HEADER_KEY_LENGTH);

        /* encode data */
        if (request.header.req_type == reqType.req_cnt)
        {
            RData_Connect data = (RData_Connect)Marshal.PtrToStructure(request.data, typeof(RData_Connect));
            byte[] rtype = BitConverter.GetBytes((int)data.conn_type);
            byte[] rname = StringToByteArray(data.name, Protocol.NICKNAME_LENGTH);
            byte[] rpsswd = StringToByteArray(data.name, Protocol.PASSWORD_LENGTH);

            byte[] content = new byte[oft + rtype.Length + NICKNAME_LENGTH + NICKNAME_LENGTH];
            System.Buffer.BlockCopy(header, 0, content, 0, oft);
            System.Buffer.BlockCopy(rtype, 0, content, oft, rtype.Length);
            System.Buffer.BlockCopy(rname, 0, content, oft + rtype.Length, rname.Length);
            System.Buffer.BlockCopy(rpsswd, 0, content, oft + rtype.Length + rname.Length, rpsswd.Length);

            
            request.header.size =(ulong)(rtype.Length + rname.Length + rpsswd.Length);
            
            hsize = BitConverter.GetBytes(request.header.size);
            System.Buffer.BlockCopy(hsize, 0, content, htype.Length, hsize.Length);

            return content;
        }
        else if (request.header.req_type == reqType.req_snd)
        {
            RData_File data = (RData_File)Marshal.PtrToStructure(request.data, typeof(RData_File));
            byte[] rid = BitConverter.GetBytes(data.id);
            byte[] rtype = BitConverter.GetBytes((int)data.file_type);
            byte[] rsize = BitConverter.GetBytes(data.size);

            byte[] content = new byte[oft + rid.Length + rtype.Length + rsize.Length];
            System.Buffer.BlockCopy(header, 0, content, 0, oft);
            System.Buffer.BlockCopy(rid, 0, content, oft, rid.Length);
            System.Buffer.BlockCopy(rtype, 0, content, oft + rid.Length, rtype.Length);
            System.Buffer.BlockCopy(rsize, 0, content, oft + rid.Length + rtype.Length, rsize.Length);

            request.header.size = (ulong)(rid.Length + rtype.Length + rsize.Length);
            hsize = BitConverter.GetBytes(request.header.size);
            System.Buffer.BlockCopy(hsize, 0, content, htype.Length, hsize.Length);

            return content;
        }
        else if (request.header.req_type == reqType.req_rcv)
        {
            
            request.header.size = (ulong)0;
            hsize = BitConverter.GetBytes(request.header.size);
            System.Buffer.BlockCopy(hsize, 0, header, htype.Length, hsize.Length);

            return header;
        }
        // else if (request.header.req_type == reqType.req_res)
        return new byte[0]; 
    }

};