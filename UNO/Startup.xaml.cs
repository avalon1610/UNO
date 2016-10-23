using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Input;

namespace UNO
{
    /// <summary>
    /// Interaction logic for Startup.xaml
    /// </summary>
    public partial class Startup : Window
    {
        public ObservableCollection<Room> RoomList { get; set; }
        internal Service service;
        public Startup()
        {
            InitializeComponent();
            service = new Service();
            RoomList = new ObservableCollection<Room>();
            listBoxRoomList.DataContext = this;
            service.rlr_event += new RoomListResultHandler(onRoomListResult);
            service.lr_event += new LoginResultHandler(onLoginResult);
        }

        void onLoginResult(LoginResult result)
        {
            this.Dispatcher.Invoke(new Action(() =>
            {
                if (result == null)
                {
                    MessageBox.Show(this, "Login or Create failed.");
                    return;
                }

                MainWindow main = new MainWindow(this, textBoxName.Text, result);
                main.Show();
                this.Hide();
            }));
        }

        void onRoomListResult(List<RoomInfo> room_info)
        {
            this.Dispatcher.Invoke(new Action(() =>
            {
                RoomList.Clear();
                foreach (var info in room_info)
                {
                    RoomList.Add(new Room(info.number, info.name, info.locked));
                }
            }));
        }

        private bool GetName(out string name)
        {
            name = "";
            if (textBoxName.Text.Length == 0)
            {
                MessageBox.Show(this, "You must enter a name.");
                return false;
            }

            name = textBoxName.Text;
            return true;
        }

        private void buttonCreate_Click(object sender, RoutedEventArgs e)
        {
            string name;
            if (GetName(out name))
                service.CreateRoom(name);
        }

        private void FlushRoomList()
        {
            service.GetRoomList();
        }

        private void listBoxRoomList_MouseRightButtonUp(object sender, MouseButtonEventArgs e)
        {
            FlushRoomList();
        }

        private void buttonLogin_Click(object sender, RoutedEventArgs e)
        {
            string name;
            if (!GetName(out name))
                return;
            Room room = listBoxRoomList.SelectedItem as Room;
            if (room == null)
            {
                MessageBox.Show(this, "Select a room first.");
                return;
            }

            if (room.Locked.Length != 0)
            {
                Password pwdDialog = new Password();
                if (!pwdDialog.ShowDialog().Value)
                    return;
                string password = pwdDialog.textboxPwd.Password;
                service.Login(room.Id, name, password);
            }
            else
                service.Login(room.Id, name);
        }

        private void listBoxRoomList_Loaded(object sender, RoutedEventArgs e)
        {
            FlushRoomList();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            service.Shutdown();
        }
    }

    public class Room
    {
        public Room(int id, string name, bool locked)
        {
            this.Id = id;
            this.Name = name;
            this.Locked = locked ? "*" : "";
        }

        public string Locked { get; set; }
        public int Id { get; set; }
        public string Name { get; set; }
        public int Timeout { get; set; }
    }

}


