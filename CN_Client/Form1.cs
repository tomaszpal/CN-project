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
using System.Runtime.InteropServices;


namespace DayTime
{
    public partial class Form1 : Form
    {
        private Form obj;
        delegate void setThreadedTextBoxCallback(String text);
        delegate void setThreadedStatusLabelCallback(String text);
        delegate void setThreadedButtonCallback(bool status);
        delegate void setThreadedSendButtonCallback(bool status);
        delegate void setThreadedReceiveButtonCallback(bool status);
        delegate void setThreadedTasksDoneCallback(string text);
        delegate void setThreadedTasksProgressCallback();

        Client client = new Client();

        public Form1()
        {
            InitializeComponent();
            this.obj = this;
        }

        /* --------SETTING OBJECTS---------------------*/
        private void setThreadedTextBox(String text) {
            if (this.textBoxStatus.InvokeRequired) {
                setThreadedTextBoxCallback textBoxCallback = new setThreadedTextBoxCallback(setThreadedTextBox);
                this.obj.Invoke(textBoxCallback, text);
            }
            else {
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

        private void setThreadedSendButton(bool status)
        {
            if (this.buttonSend.InvokeRequired)
            {
                setThreadedButtonCallback buttonCallback = new setThreadedButtonCallback(setThreadedSendButton);
                this.obj.Invoke(buttonCallback, status);
            }
            else
            {
                this.buttonSend.Enabled = status;
            }
        }

        private void setThreadedReceiveButton(bool status)
        {
            if (this.buttonReceive.InvokeRequired)
            {
                setThreadedButtonCallback buttonCallback = new setThreadedButtonCallback(setThreadedReceiveButton);
                this.obj.Invoke(buttonCallback, status);
            }
            else
            {
                this.buttonReceive.Enabled = status;
            }
        }



        //adds text line to text box TasksDone
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

        //refreshes TasksProgress
        private void setThreadedTasksProgress()
        {
            if (this.statusStrip.InvokeRequired)
            {
                setThreadedTasksProgressCallback statusTasksProgressCallback = new setThreadedTasksProgressCallback(setThreadedTasksProgress);
                this.obj.Invoke(statusTasksProgressCallback);
            }
            else
            {
                tasksProgress.Text = "";
                foreach(int i in client.tasksProgress)
                {
                    tasksProgress.AppendText(i.ToString() + Environment.NewLine);
                }
            }
        }

        /* --------CONNECTION FUNCTIONS---------------------*/
        private void feedback(ref Protocol.Request response)
        {
            setThreadedStatusLabel("Managing response");
            setThreadedButton(true);

            if (response.header.req_type == Protocol.reqType.req_res)
            { //some error responses
                Protocol.RData_Response ans_data = (Protocol.RData_Response)Marshal.PtrToStructure(response.data, typeof(Protocol.RData_Response));
                if (ans_data.res_type == Protocol.resType.res_ok)
                {
                    if (ans_data.id == -1)
                    {
                        setThreadedStatusLabel("Connected succesfully");
                        setThreadedSendButton(true);

                    }
                    else //file accepted
                    {
                        
                        client.tasksProgress.Add(ans_data.id);
                        setThreadedTasksProgress();
                        setThreadedStatusLabel("File accepted");
                        setThreadedReceiveButton(true);
                    }
                }

                else if (ans_data.res_type == Protocol.resType.res_empty)
                {
                    setThreadedStatusLabel("No files to download");
                }
                else if (ans_data.res_type == Protocol.resType.res_fail)
                {
                    setThreadedStatusLabel("Operation failed on server");
                }
                else if (ans_data.res_type == Protocol.resType.res_full)
                {
                    setThreadedStatusLabel("Server is busy. Try again later");
                }
                else
                { //other srver response
                    setThreadedStatusLabel("Unsupported request type");
                }
            }
            else if (response.header.req_type == Protocol.reqType.req_snd)
            { //there is a file
                Protocol.RData_File ans_data = (Protocol.RData_File)Marshal.PtrToStructure(response.data, typeof(Protocol.RData_File));
                taskDone(ref ans_data);

            }
            else
            { //other srver response
                setThreadedStatusLabel("Unsupported request type");
            }
        }

        private void ReceiveRestCallback(IAsyncResult ar)
        {
            try
            {
                /* retrieve the SocketStateObject */
                SocketStateObject state = (SocketStateObject)ar.AsyncState;
                Socket socketFd = state.m_SocketFd;

                /* read data */
                int readSize = socketFd.EndReceive(ar);
                state.m_Data = Protocol.combine(state.m_Data, state.m_DataBuf, readSize);
                state.m_DataBuf = new byte[state.size - state.m_Data.Length + 21];
                // receive the rest of header
                if (state.m_Data.Length - 21 < state.size)
                {
                    socketFd.BeginReceive(state.m_DataBuf, 0, state.size - state.m_Data.Length + 21, 0, new AsyncCallback(ReceiveRestCallback), state);
                }
                else
                {
                    Protocol.Request req = new Protocol.Request();
                    Protocol.req_decode(ref req, state.m_Data);
                    /* manage response */
                    feedback(ref req);
                    setThreadedSendButton(true);
                    setThreadedReceiveButton(true);
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("Exception:\t\n" + exc.Message.ToString());
                setThreadedStatusLabel("Check \"Server Info\" and try again!");
                setThreadedButton(true);
            }
        }

        private void ReceiveCallback(IAsyncResult ar) {
            try
            {
                /* retrieve the SocketStateObject */
                SocketStateObject state = (SocketStateObject)ar.AsyncState;
                Socket socketFd = state.m_SocketFd;
                
                /* read data */
                int readSize = socketFd.EndReceive(ar);
                state.m_Data = Protocol.combine(state.m_Data, state.m_DataBuf, readSize);
                state.m_DataBuf = new byte[state.size - state.m_Data.Length];
                // receive the rest of header
                if (state.m_Data.Length < state.size)
                {
                    socketFd.BeginReceive(state.m_DataBuf, 0, state.size - state.m_Data.Length, 0, new AsyncCallback(ReceiveCallback), state);
                }
                else
                {
                    Protocol.Request req = new Protocol.Request();
                    if (state.m_Data.Length != state.size)
                    {
                        throw new Exception("Wrong size of data");
                    }
                    Protocol.req_decode(ref req, state.m_Data, true);
                    state.size = (int)req.header.size;
                    state.m_DataBuf = new byte[state.size];
                    /*begin receiving the rest */
                    socketFd.BeginReceive(state.m_DataBuf, 0, state.size, 0, new AsyncCallback(ReceiveRestCallback), state);
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
                setThreadedTextBox("Sent " + bytesSent.ToString() + " bytes to server.");
                state.size -= bytesSent;

                if (state.size > 0)
                {
                    client.s_socket.BeginSend(state.m_DataBuf, state.m_DataBuf.Length - state.size, state.size, 0, new AsyncCallback(SendCallback), state);
                }
                else
                {
                    state.size = 21;
                    state.m_DataBuf = new byte[state.size];
                    state.m_Data = new byte[0];
                    /* begin receiving the header */
                    socketFd.BeginReceive(state.m_DataBuf, 0, state.size, 0, new AsyncCallback(ReceiveCallback), state);
                }
            }
            catch (Exception e) {
                setThreadedStatusLabel(e.ToString());
            }
        }

        private void sendRequest(ref Protocol.Request req)
        {
            /* send */
            SocketStateObject state = new SocketStateObject();
            state.m_SocketFd = this.client.s_socket;
            state.m_DataBuf = Protocol.req_encode(ref req);

            setThreadedStatusLabel("Wait! Sending information...");
            state.size = state.m_DataBuf.Length;
            client.s_socket.BeginSend(state.m_DataBuf, 0, state.size, 0, new AsyncCallback(SendCallback), state);
        }


        private void ConnectCallback(IAsyncResult ar) {
            try {
                /* retrieve the socket from the state object */
                Socket socketFd = (Socket)ar.AsyncState;
                /* complete the connection */
                socketFd.EndConnect(ar);
                this.client.s_socket = socketFd;
                setThreadedStatusLabel("Connection established");
                setThreadedButton(false);
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
                this.client.connectDone.WaitOne();

                /* send client information */
                Protocol.Request req;
                req = new Protocol.Request();

                //creating data
                Protocol.RData_Connect data = new Protocol.RData_Connect();
                data.conn_type = Protocol.connType.conn_client;
                data.name = "name";
                data.password = "psswd";
                req.data = Marshal.AllocHGlobal(Marshal.SizeOf(data));
                Marshal.StructureToPtr(data, req.data, false);

                //creating header
                req.header = new Protocol.Header();
                req.header.req_type = Protocol.reqType.req_cnt;
                req.header.key = "12345678\0";

                sendRequest(ref req);
                this.client.sendDone.WaitOne();
            }
            catch (Exception exc) {
                MessageBox.Show("Exception:\t\n" + exc.Message.ToString());
                setThreadedStatusLabel("Check \"Server Info\" and try again!");
                setThreadedButton(true);
            }
        }



        private void taskDone(ref Protocol.RData_File datafile)
        {
            if (!client.tasksProgress.Contains(datafile.id)) {
                setThreadedStatusLabel("Incorrect received file id");
                return;
            }
            else
            {
                //delete from in progress
                client.tasksProgress.Remove(datafile.id);
                setThreadedTasksProgress();
                
                
                string filename = datafile.id.ToString() + "_res.txt";
                string path = Directory.GetParent(Directory.GetCurrentDirectory()).Parent.FullName + @"\results\" + filename;
                setThreadedTasksDone(datafile.id.ToString() + ": " + path);
                try
                {
                    if (File.Exists(path))
                    {
                        File.Delete(path);
                    }

                    // Create the file.
                    using (FileStream fs = File.Create(path))
                    {
                        // Add text to the file.
                        Byte[] info = Protocol.StringToByteArray(datafile.data, (int) datafile.size);
                        fs.Write(info, 0, info.Length);
                    }
                }
                catch (Exception exc)
                {
                    MessageBox.Show("Exception:\t\n" + exc.Message.ToString());
                }
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
                    /* get DNS host information and connect */
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
            fdlg.Title = "Choose python file";
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
                    setThreadedSendButton(false);
                    setThreadedReceiveButton(false);
                    using (StreamReader sr = new StreamReader(filename)) {
                        String line = sr.ReadToEnd();
                        Protocol.Request req;
                        req = new Protocol.Request();
                        //creating data
                        Protocol.RData_File data_file = new Protocol.RData_File();
                        data_file.file_type = radioButtonP2.Checked ? Protocol.fileType.file_py2_script : Protocol.fileType.file_py3_script;
                        data_file.size = (ulong)line.Length;
                        data_file.data = line;
                        req.data = Marshal.AllocHGlobal(Marshal.SizeOf(data_file));
                        Marshal.StructureToPtr(data_file, req.data, false);
                        //creating header
                        req.header = new Protocol.Header();
                        req.header.req_type = Protocol.reqType.req_snd;
                        req.header.size = (ulong)Marshal.SizeOf(data_file.GetType());
                        req.header.key = "12345678";
                        sendRequest(ref req);
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
                setThreadedSendButton(false);
                setThreadedReceiveButton(false);
                Protocol.Request req = new Protocol.Request();
                //creating data 
                req.data = new IntPtr();
                //creating header
                req.header = new Protocol.Header();
                req.header.req_type = Protocol.reqType.req_rcv;
                req.header.size = 0;
                req.header.key = "12345678";

                sendRequest(ref req);
            }
            catch (Exception exc)  {
                    setThreadedStatusLabel("Exception:\t\n" + exc.Message.ToString());
            }
        }
    }

    
    public class SocketStateObject
    {
        //size of receive buffer
        public const int BUF_SIZE = 64;
        //receive buffer
        public byte[] m_DataBuf;
        //received byte array
        public byte[] m_Data;
        //number of bytes to receive
        public int size;
        //Client socket
        public Socket m_SocketFd = null;
    }

    public class Client
    {
        public const int MAX_TASKS_NUMBER = 5;
        public String[] balance = new String[MAX_TASKS_NUMBER];
        public Socket s_socket;
        public List<int> tasksProgress = new List<int>();

        public System.Threading.ManualResetEvent connectDone = new System.Threading.ManualResetEvent(false);
        public System.Threading.ManualResetEvent sendDone = new System.Threading.ManualResetEvent(false);
        public System.Threading.ManualResetEvent receiveDone = new System.Threading.ManualResetEvent(false);
    }
}