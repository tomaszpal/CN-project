using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Protocol
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
       [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
       public struct Header
        {
           public reqType req_type;
           public ulong size;
           [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 8)]
           public string key;
        };

    /* Defines request.                                             */
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct Request
        {
            public Header header;
            public IntPtr data;
        };

    /* Defines login request data.                                   */
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct RData_Connect
        {
            public connType conn_type;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
            public string name;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
            public string password;
        };

    /* Defines file sending request data.                            */
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct RData_File
        {
            public int id;
            public fileType file_type;
            public ulong size;
            public IntPtr data;
        };

    /* Defines server response                                       */
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct RData_Response
        {
            public resType res_type;
            public int id;
        };
/*
    [DllImport("kernel32.dll", EntryPoint = "CopyMemory", SetLastError = false)]
    public static extern void CopyMemory(IntPtr dest, IntPtr src, uint count);

    public Request req_encode(reqType type, IntPtr data, string key) {
        Request request;
        request.header.req_type = type;
        request.header.key = key.Substring(0,7);
        if (type == reqType.req_cnt || type == reqType.req_res) {
            request.header.size = (ulong) Marshal.SizeOf(typeof(RData_Connect));
            request.data = Marshal.AllocHGlobal(Marshal.SizeOf(request.header.size));
            CopyMemory(request.data, data, request.header.size);  
        }
        if (type == reqType.req_snd) {
            RData_File tmp = (RData_Connect)Marshal.PtrToStructure(data, typeof(RData_Connect);
            request.header.size = (ulong)Marshal.SizeOf(typeof(RData_Connect)) + tmp.size;
            request.data = Marshal.AllocHGlobal(Marshal.SizeOf(request.header.size));
            IntPtr ptr = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(request.data, ptr, true);
            Marshal.Copy(ptr, arr, 0, size);
        }
        return request;
    };
*/
};