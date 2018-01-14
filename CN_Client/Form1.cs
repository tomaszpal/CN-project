using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
using System.IO;
using Protocol;
using System.Runtime.InteropServices;



/* mmm
 * $Id: Form1.cs,v 1.1 2006/10/24 19:32:59 mkalewski Exp $
 */

namespace DayTime
{
    public partial class Form1 : Form
    {
        private Form obj;
        delegate void setThreadedTextBoxCallback(String text);
        delegate void setThreadedStatusLabelCallback(String text);
        delegate void setThreadedButtonCallback(bool status);
        delegate void setThreadedSendButtonCallback(bool status);
        delegate void setThreadedTasksDoneCallback(string text);
        delegate void setThreadedTasksProgressCallback(string text);

        Client client = new Client();

        public Form1()
        {
            InitializeComponent();
            this.obj = this;
            this.buttonBrowse.Click += new System.EventHandler(this.buttonBrowse_Click);
            this.buttonSend.Click += new System.EventHandler(this.buttonSend_Click);
            this.buttonConnect.Click += new System.EventHandler(this.buttonConnect_Click);
            this.buttonReceive.Click += new System.EventHandler(this.buttonReceive_Click);
            tasksProgress.Multiline = true;
            tasksProgress.ScrollBars = ScrollBars.Vertical;
            tasksProgress.WordWrap = true;

            tasksDone.Multiline = true;
            tasksDone.ScrollBars = ScrollBars.Vertical;
            tasksDone.WordWrap = true;

            

        }

        /* --------SETTING OBJECTS---------------------*/
        private void setThreadedTextBox(String text) {
            if (this.textBoxStatus.InvokeRequired) {
                setThreadedTextBoxCallback textBoxCallback = new setThreadedTextBoxCallback(setThreadedTextBox);
                this.obj.Invoke(textBoxCallback, text);
            }
            else {
                //text = text.Substring(0, text.Length - 2);
                this.textBoxStatus.Text = text;
            }
        }

        private void setThreadedStatusLabel(String text) {
            if (this.statusStrip.InvokeRequired) {
                setThreadedStatusLabelCallback statusLabelCallback = new setThreadedStatusLabelCallback(setThreadedStatusLabel);
                this.obj.Invoke(statusLabelCallback, text);
            }
            else {
                this.toolStripStatusLabel1.Text = text;
            }
        }

        private void setThreadedButton(bool status)
        {
            if (this.buttonConnect.InvokeRequired)
            {
                setThreadedButtonCallback buttonCallback = new setThreadedButtonCallback(setThreadedButton);
                this.obj.Invoke(buttonCallback, status);
            }
            else
            {
                this.buttonConnect.Enabled = status;
            }
        }
        private void setSendButton(bool status)
        {
            if (this.buttonSend.InvokeRequired)
            {
                setThreadedButtonCallback buttonCallback = new setThreadedButtonCallback(setSendButton);
                this.obj.Invoke(buttonCallback, status);
            }
            else
            {
                this.buttonSend.Enabled = status;
            }
        }

        private void setThreadedTasksDone(String text)
        {
            if (this.statusStrip.InvokeRequired)
            {
                setThreadedTasksDoneCallback statusTasksDoneCallback = new setThreadedTasksDoneCallback(setThreadedTasksDone);
                this.obj.Invoke(statusTasksDoneCallback, text);
            }
            else
            {
                if (tasksDone.Text.Length == 0)
                    tasksDone.Text = text;
                else
                    tasksDone.AppendText(Environment.NewLine + text);
            }
        }

        private void setThreadedTasksProgress(String text)
        {
            if (this.statusStrip.InvokeRequired)
            {
                setThreadedTasksProgressCallback statusTasksProgressCallback = new setThreadedTasksProgressCallback(setThreadedTasksProgress);
                this.obj.Invoke(statusTasksProgressCallback, text);
            }
            else
            {
                tasksDone.Text = text;
                
            }
        }

        /* --------CONNECTION FUNCTIONS---------------------*/
        private void ReceiveCallback(IAsyncResult ar)
        {
            try
            {
                /* retrieve the SocketStateObject */
                SocketStateObject state = (SocketStateObject)ar.AsyncState;
                Socket socketFd = state.m_SocketFd;

                /* read data */
                int size = socketFd.EndReceive(ar);

                if (size > 0) {
                    state.m_StringBuilder.Append(Encoding.ASCII.GetString(state.m_DataBuf, 0, size));

                    /* get the rest of the data */
                    socketFd.BeginReceive(state.m_DataBuf, 0, SocketStateObject.BUF_SIZE, 0, new AsyncCallback(ReceiveCallback), state);
                }
                else {
                    /* all the data has arrived */
                    if (state.m_StringBuilder.Length > 1) {
                        setThreadedTextBox(state.m_StringBuilder.ToString());
                        setThreadedStatusLabel("Done.");
                        setThreadedButton(true);
                        this.client.receiveDone.Set();
                        /* shutdown and close socket */
                        socketFd.Shutdown(SocketShutdown.Both);
                        socketFd.Close();
                    }
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("Exception:\t\n" + exc.Message.ToString());
                setThreadedStatusLabel("Check \"Server Info\" and try again!");
                setThreadedButton(true);
            }
        }


        private void SendCallback(IAsyncResult ar) {
            try {
                /* retrieve the SocketStateObject */
                SocketStateObject state = (SocketStateObject)ar.AsyncState;
                Socket socketFd = state.m_SocketFd;
                // Complete sending the data to the remote device.  
                int bytesSent = socketFd.EndSend(ar);
                setThreadedTextBox("Sent " + bytesSent.ToString() +" bytes to server.");
                // Signal that all bytes have been sent.  
                
                /* begin receiving the server answer */
                socketFd.BeginReceive(state.m_DataBuf, 0, SocketStateObject.BUF_SIZE, 0, new AsyncCallback(ReceiveCallback), state);
               // this.client.receiveDone.WaitOne();
                this.client.sendDone.Set();
            }
            catch (Exception e) {
                setThreadedStatusLabel(e.ToString());
            }
        }

        private Request sendRequest(Request req)
        {
            SocketStateObject state = new SocketStateObject();
            /* encode */

            //state.m_DataBuf = czary mary(req)

            /* send */

            state.m_SocketFd = this.client.s_socket;
           // setThreadedStatusLabel("Wait! Sending information...");
            //socketFd.BeginSend(state.m_DataBuf, 0, SocketStateObject.BUF_SIZE, 0, new AsyncCallback(SendCallback), state);
            //this.client.sendDone.WaitOne();

            //receive
            //decode
            return req;
        }


        private void ConnectCallback(IAsyncResult ar) {
            try {
                /* retrieve the socket from the state object */
                Socket socketFd = (Socket) ar.AsyncState;
                /* complete the connection */
                socketFd.EndConnect(ar);

                /* create the SocketStateObject */
                // SocketStateObject state = new SocketStateObject();
                //state.m_SocketFd = socketFd;

                // setThreadedStatusLabel("Wait! Sending connection information...");
                //socketFd.BeginSend(state.m_DataBuf, 0, SocketStateObject.BUF_SIZE, 0, new AsyncCallback(SendCallback), state);
                //sendDone.WaitOne();
                this.client.s_socket = socketFd;
                setThreadedStatusLabel("Connection established");
                this.client.connectDone.Set();
                
            }
            catch (Exception exc)
            {
                MessageBox.Show("Exception:\t\n" + exc.Message.ToString());
                setThreadedStatusLabel("Check \"Server Info\" and try again!");
                setThreadedButton(true);
            }
        }

        private void GetHostEntryCallback(IAsyncResult ar) {
            try {
                IPHostEntry hostEntry = null;
                IPAddress[] addresses = null;
                Socket socketFd = null;
                IPEndPoint endPoint = null;

                /* complete the DNS query */
                hostEntry = Dns.EndGetHostEntry(ar);
                addresses = hostEntry.AddressList;

                /* create a socket */
                socketFd = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

                /* remote endpoint for the socket */
                endPoint = new IPEndPoint(addresses[0], Int32.Parse(this.textBoxPort.Text.ToString()));

                setThreadedStatusLabel("Wait! Connecting...");


                /* connect to the server */
                socketFd.BeginConnect(endPoint, new AsyncCallback(ConnectCallback), socketFd);
                // this.client.connectDone.WaitOne();
                /* send client information */
                Request req, ans_req;
                req = new Request();
                //creating data
                RData_Connect data = new RData_Connect();
                data.conn_type = connType.conn_client;
                data.name = "";
                data.password = "";
                req.data = Marshal.AllocHGlobal(Marshal.SizeOf(data));
                Marshal.StructureToPtr(data, req.data, false);
                //creating header
                req.header = new Header();
                req.header.req_type = reqType.req_cnt;
                req.header.size = (ulong)Marshal.SizeOf(data.GetType());
                req.header.key = "12345678";

                ans_req = sendRequest(req);

            }
            catch (Exception exc) {
                MessageBox.Show("Exception:\t\n" + exc.Message.ToString());
                setThreadedStatusLabel("Check \"Server Info\" and try again!");
                setThreadedButton(true);
            }
        }



        private void taskDone(RData_File datafile)
        {
            string filename = "";

            if (!client.tasksProgress.TryGetValue(datafile.id, out filename)) {
                setThreadedStatusLabel("Incorrect received file id");
                return;
            }
            //delete from in progress
            client.tasksProgress.Remove(datafile.id);
            //add to done tasks
            setThreadedTasksDone(datafile.id.ToString());


            filename = Path.GetFileNameWithoutExtension(filename) + datafile.id.ToString() + "_res.txt";
            string path = Directory.GetParent(Directory.GetCurrentDirectory()).Parent.FullName + @"results\" + filename;

            String text = Marshal.PtrToStringAnsi(datafile.data, (int)datafile.size); //@TODO cos z ulongiem

            try {
                if (File.Exists(path)) {
                    File.Delete(path);
                }

                // Create the file.
                using (FileStream fs = File.Create(path)) {
                    // Add text to the file.
                    Byte[] info = new UTF8Encoding(true).GetBytes(text);
                    fs.Write(info, 0, info.Length);
                }  
            }
            catch (Exception exc) {
                MessageBox.Show("Exception:\t\n" + exc.Message.ToString());
            }
        
        }

        /*------------------------BUTTONS-----------------------------*/
        /* Connect button */
        private void buttonConnect_Click(object sender, EventArgs e) {
            try {
                setThreadedButton(false);
                setThreadedTextBox("");
                setThreadedStatusLabel("Wait! DNS query...");

                if (this.textBoxAddr.Text.Length > 0 && this.textBoxPort.Text.Length > 0) {
                    /* get DNS host information */ /* and connect */
                    Dns.BeginGetHostEntry(this.textBoxAddr.Text.ToString(), new AsyncCallback(GetHostEntryCallback), null);
                  
                }
                else {
                    if (this.textBoxAddr.Text.Length <= 0) MessageBox.Show("No server address!");
                    else
                        if (this.textBoxPort.Text.Length <= 0) MessageBox.Show("No server port number!");
                    setThreadedButton(true);
                    setThreadedStatusLabel("Check \"Server Info\" and try again!");
                }
            }
            catch (Exception exc) {
                MessageBox.Show("Exception:\t\n" + exc.Message.ToString());
                setThreadedStatusLabel("Check \"Server Info\" and try again!");
                setThreadedButton(true);
            }
        }

        /* Browse button */
        private void buttonBrowse_Click(object sender, EventArgs e)
        {

            OpenFileDialog fdlg = new OpenFileDialog();
            fdlg.Title = "C# Corner Open File Dialog";
            fdlg.InitialDirectory = @"c:\";
            fdlg.Filter = "Python files |*.py";
            fdlg.FilterIndex = 2;
            fdlg.RestoreDirectory = true;
            if (fdlg.ShowDialog() == DialogResult.OK)
            {
                textBoxFile.Text = fdlg.FileName;
            }

        }

        /* Send button */
        private void buttonSend_Click(object sender, EventArgs e) {
            if ((radioButtonP2.Checked || radioButtonP3.Checked) && textBoxFile.Text.Length > 0) {
                string filename = textBoxFile.Text;
                try {
                    using (StreamReader sr = new StreamReader(filename)) {
                        String line = sr.ReadToEnd();
                        Request req, ans_req;
                        req = new Request();
                        //creating data
                        RData_File data_file = new RData_File();
                        data_file.file_type = radioButtonP2.Checked ? fileType.file_py2_script : fileType.file_py3_script;
                        data_file.size = (ulong)line.Length;
                        data_file.data = Marshal.StringToHGlobalUni(line);
                        req.data = Marshal.AllocHGlobal(Marshal.SizeOf(data_file));
                        Marshal.StructureToPtr(data_file, req.data, false);
                        //creating header
                        req.header = new Header();
                        req.header.req_type = reqType.req_snd;
                        req.header.size = (ulong)Marshal.SizeOf(data_file.GetType());
                        req.header.key = "12345678";
                        
                        ans_req = sendRequest(req);

                        Request response = (Request)Marshal.PtrToStructure(ans_req.data, typeof(Request));
                        if (response.header.req_type == reqType.req_res) {
                            RData_Response ans_data = (RData_Response)Marshal.PtrToStructure(response.data, typeof(RData_Response));
                            if (ans_data.res_type == resType.res_ok) {
                                setThreadedStatusLabel("File sent to server succesfully");
                                client.tasksProgress.Add(ans_data.id, filename);

                                if (tasksProgress.Text.Length == 0)
                                    tasksProgress.Text = filename;
                                else
                                    tasksProgress.AppendText(Environment.NewLine + filename);
                            }
                            else if (ans_data.res_type == resType.res_fail) {
                                setThreadedStatusLabel("Unable to read file on server");
                            }
                            else if (ans_data.res_type == resType.res_full) {
                                setThreadedStatusLabel("Server is busy. Try again later");
                            }

                        }
                        else {
                            setThreadedStatusLabel("Unsupported request type");
                        }
                    // byte[] dataBuf = Encoding.ASCII.GetBytes(data_file.ToString());
                    /* begin sending the data */
                    // m_socket.BeginSend(dataBuf, 0, dataBuf.Length, 0, new AsyncCallback(SendCallback), m_socket);
                    }
                }
                catch (Exception exc) {
                    setThreadedStatusLabel("Exception:\t\n" + exc.Message.ToString());
                }
            }
            else {
                setThreadedStatusLabel("No file or file type!");
            }
        }

        /* Receive button */
        private void buttonReceive_Click(object sender, EventArgs e) {
            try {
                Request ans_req;
                Request req = new Request();
                //creating data 
                req.data = new IntPtr();
                //creating header
                req.header = new Header();
                req.header.req_type = reqType.req_rcv;
                req.header.size = 0;
                req.header.key = "12345678";

                ans_req = sendRequest(req);

                Request response = (Request)Marshal.PtrToStructure(ans_req.data, typeof(Request));
                if (response.header.req_type == reqType.req_res) { //some error responses
                    RData_Response ans_data = (RData_Response)Marshal.PtrToStructure(response.data, typeof(RData_Response));
                    if (ans_data.res_type == resType.res_empty) {
                        setThreadedStatusLabel("No files to download");
                    }
                    else if (ans_data.res_type == resType.res_fail) {
                        setThreadedStatusLabel("Unable to read file on server");
                    }

                }
                else if (response.header.req_type == reqType.req_snd) { //there is a file
                    RData_File ans_data = (RData_File)Marshal.PtrToStructure(response.data, typeof(RData_File));
                    taskDone(ans_data);
                }
                else { //other srver response
                    setThreadedStatusLabel("Unsupported request type");
                }
            }
            catch (Exception exc)  {
                    setThreadedStatusLabel("Exception:\t\n" + exc.Message.ToString());
            }
            
        }

    }



    public class SocketStateObject
    {
        //size of receive buffer
        public const int BUF_SIZE = 1024;
        //receive buffer
        public byte[] m_DataBuf = new byte[BUF_SIZE];
        //received data string
        public StringBuilder m_StringBuilder = new StringBuilder();
        //Client socket
        public Socket m_SocketFd = null;
    }

    public class Client
    {
        public const int MAX_TASKS_NUMBER = 5;
        public String[] balance = new String[MAX_TASKS_NUMBER];

        public System.Threading.ManualResetEvent connectDone = new System.Threading.ManualResetEvent(false);
        public System.Threading.ManualResetEvent sendDone = new System.Threading.ManualResetEvent(false);
        public System.Threading.ManualResetEvent receiveDone = new System.Threading.ManualResetEvent(false);

        public Socket s_socket;

        public Dictionary<int, string> tasksProgress = new Dictionary<int, string>();
    }
}