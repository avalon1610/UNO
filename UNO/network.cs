using System;
using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;

namespace UNO
{
    class Network : IDisposable
    {
        TcpClient client = new TcpClient();
        // default setting
        string ip = "127.0.0.1";
        int port = 7777;
        AsyncCallback ConnectCallback, ReceiveCallback;
        int disposed = 0;
        public bool connected 
        { 
            get 
            { 
                if (client == null)
                    return false;
                return client.Client.Connected;
            }
        }
        public void Dispose()
        {
            if (Interlocked.Increment(ref disposed) == 1)
            {
                if (client != null)
                {
                    if (connected)
                        client.Client.Disconnect(true);
                    client.Close();
                    client = null;
                }
                GC.SuppressFinalize(this);
            }
        }

        public Network(Service service)
        {
            this.service = service;
            if (service.ini != null)
            {
                if (!service.ini.KeyExists("server_ip"))
                    service.ini.Write("server_ip", ip);
                else
                    ip = service.ini.Read("server_ip");

                if (!service.ini.KeyExists("server_port"))
                    service.ini.Write("server_port", Convert.ToString(port));
                else
                    port = Convert.ToInt32(service.ini.Read("server_port"));
            }
            ConnectCallback = new AsyncCallback(OnConnected);
            ReceiveCallback = new AsyncCallback(OnReceived);
            task = new Task(ProcessData);
            IPAddress addr;
            if (!IPAddress.TryParse(ip, out addr))
                return;
            client.BeginConnect(addr, port, ConnectCallback, null);
        }

        class State
        {
            public const int BUFFER_SIZE = (short.MaxValue / 8 + 1);  // 4096
            public byte[] buffer = new byte[BUFFER_SIZE];
        }

        public void Send(byte[] buffer)
        {
            if (disposed != 0)
                return;
            if (!connected)
                return;
            try
            {
                client.Client.Send(buffer);
            }
            catch (System.Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        void OnConnected(IAsyncResult result)
        {
            if (disposed != 0)
                return;
            try
            {
                client.EndConnect(result);
                State state = new State();
                client.Client.BeginReceive(state.buffer, 0, State.BUFFER_SIZE, 0, ReceiveCallback, state);
            }
            catch (System.Net.Sockets.SocketException ex)
            {
                MessageBox.Show(ex.Message);
                if (ex.SocketErrorCode == SocketError.ConnectionRefused)
                    Environment.Exit(0);
            }
            catch (System.Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        ConcurrentQueue<byte[]> bufferQueue = new ConcurrentQueue<byte[]>();
        void OnReceived(IAsyncResult result)
        {
            if (disposed != 0)
                return;
            try
            {
                int readLength = client.Client.EndReceive(result);
                State state = result.AsyncState as State;
                if (readLength <= 0)    // socket disconnect
                {
                    Dispose();
                    return;
                }

                if (readLength > State.BUFFER_SIZE)
                {
                    throw new Exception("receive data too large.");
                }

                byte[] buf = new byte[readLength];
                Buffer.BlockCopy(state.buffer, 0, buf, 0, buf.Length);
                bufferQueue.Enqueue(buf);
                if (task.Status != TaskStatus.Running)
                {
                    task = new Task(ProcessData);
                    task.Start();
                }
            }
            catch (System.Exception ex)
            {
                MessageBox.Show(ex.Message);
                Dispose();
            }
            finally
            {
                if (disposed == 0)
                {
                    var new_state = new State();
                    client.Client.BeginReceive(new_state.buffer, 0, State.BUFFER_SIZE, 0, ReceiveCallback, new_state);
                }
            }
        }

        Task task;
        Service service;
        void ProcessData()
        {
            try
            {
                byte[] buffer;
                while (bufferQueue.Count != 0)
                {
                    if (!bufferQueue.TryDequeue(out buffer))
                        throw new Exception("buffer dequeue failed.");
                    service.Process(buffer);
                }
            }
            catch (System.Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }
    }
}
