using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows;
using System.Windows.Input;

namespace UNO
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private HandScene hand_scene;
        private UserScene user_scene;
        protected override void OnClosed(EventArgs e)
        {
            base.OnClosed(e);
            service.user_status_event -= user_scene.OnUserInfoUpdated;
            service.room_status_event -= OnRoomDetailUpdated;
            service.play_status_event -= user_scene.OnPlayStateUpdated;
            service.dcr_event -= hand_scene.OnCardsUpdated;
            service.pcr_event -= OnPlayCardResult;
            service.chat_event -= OnChatMessage;
            service.p4a_event -= OnPlus4Assgin;
            service.dc_event -= hand_scene.OnDoubtCardInfo;
            service.sr_event -= OnScoreResult;
            this.hand_scene.Dispose();
            this.user_scene.Dispose();

        }

        public int my_id;
        string my_name;
        public RoomDetailEx room_detail { get; set; }
        Startup startup_window;
        Service service;

        public MainWindow(Startup startup, string myName, LoginResult result)
        {
            InitializeComponent();
            hand_scene = new HandScene(this);
            user_scene = new UserScene(this);
            startup_window = startup;
            service = startup_window.service;
            service.user_status_event += user_scene.OnUserInfoUpdated;
            service.user_status_event += OnFirstUserInfoUpdated;
            service.room_status_event += OnRoomDetailUpdated;
            service.play_status_event += user_scene.OnPlayStateUpdated;
            service.dcr_event += hand_scene.OnCardsUpdated;
            service.pcr_event += OnPlayCardResult;
            service.chat_event += OnChatMessage;
            service.p4a_event += OnPlus4Assgin;
            service.dc_event += hand_scene.OnDoubtCardInfo;
            service.sr_event += OnScoreResult;
            my_name = myName;
            my_id = result.myID;
            textboxRoomTimeout.DataContext = this;
            textboxRoomName.DataContext = this;
            textblockRoomState.DataContext = this;
            richtextboxChat.DataContext = this;
            if (result.status_info.type == StatusInfo.StatusType.Room)
            {
                room_detail = new RoomDetailEx(result.status_info.room_detail);
                this.Dispatcher.Invoke(new Action(() => this.Title = result.status_info.room_detail.name));
            }
            try
            {
                this.d2dHand.Scene = this.hand_scene;
                this.d2dUser.Scene = this.user_scene;
            }
            catch (System.Exception)
            {
                MessageBox.Show("Unable to create a Direct2D and/or Direct3D device.",
                    "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }
            this.hand_scene.IsAnimating = true;
            this.user_scene.IsAnimating = true;
        }

        void OnFirstUserInfoUpdated(List<UserInfo> info, bool allUser)
        {
            if (allUser && info.Count != 1)
            {
                this.Dispatcher.Invoke(new Action(() =>
                {
                    textboxRoomName.IsEnabled = false;
                    textboxRoomTimeout.IsEnabled = false;
                    passwordboxRoomPasswd.IsEnabled = false;
                    buttonRoomModify.IsEnabled = false;
                }));
            }

            service.user_status_event -= OnFirstUserInfoUpdated;
        }

        void OnScoreResult(List<ScoreInfo> info)
        {
            if (info == null || info.Count == 0)
                return;
            this.Dispatcher.Invoke(new Action(() =>
            {
                Scoreboard board = new Scoreboard(info, user_scene);
                board.Show();
            }));
        }

        public bool Plus4Assigned = false;
        void OnPlus4Assgin(int id)
        {
            if (id == my_id)
            {
                Plus4Assigned = true;
                AddLog("I'm been assgined plus 4");
            }
        }

        void AddLog(string msg)
        {
            this.Dispatcher.Invoke(new Action(() =>
            {
                richtextboxChat.AppendText(msg + "\r");
                richtextboxChat.ScrollToEnd();
            }));
        }

        static DateTime epoch = new DateTime(1970, 1, 1);
        void OnChatMessage(long time, int id, string content)
        {
            DateTime dt = epoch.AddTicks(time * 10).ToLocalTime();
            string t = string.Format("{0} {1}", dt.ToLongTimeString(), dt.Millisecond);
            string msg = string.Format("[{0}][{1}]:{2}", t, user_scene.FindUser(id), content);
            AddLog(msg);
            // todo: render system broadcast to game
        }

        private void OnRoomDetailUpdated(RoomDetail info)
        {
            room_detail.Update(info);
            if (RoomDetail.RoomState.Wait == info.state)
            {
                GameRunning = false;
                user_scene.ResetPlayState();
                user_scene.ResetUserState();
                hand_scene.ResetHand();
            }
            else
            {
                GameRunning = true;
            }
            this.Dispatcher.Invoke(new Action(() => this.Title = info.name));
        }

        public bool GameRunning { get; private set; }
        private void Window_Closed(object sender, System.EventArgs e)
        {
            startup_window.service.Reset();
            startup_window.Show();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            textblockMyID.Text = Convert.ToString(my_id);
            textblockMyName.Text = my_name;
            service.WaitGameWindow.Release();
        }

        private void buttonRoomModify_Click(object sender, RoutedEventArgs e)
        {
            string pop = "name: " + room_detail.name + "\n" +
                "timeout: " + room_detail.timeout + "\n" +
                "password: " + passwordboxRoomPasswd.Password;
            MessageBox.Show(this, pop);
            RoomDetail detail = new RoomDetail();
            detail.password = passwordboxRoomPasswd.Password;
            detail.name = room_detail.name;
            detail.timeout = room_detail.timeout;
            detail.number = room_detail.number;
            service.ModifyRoom(detail);
        }

        private void buttonReady_Click(object sender, RoutedEventArgs e)
        {
            service.Ready();
        }

        bool Plus4WillAssign = false;
        private void d2dHand_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            int second_card = -1;
            int card = hand_scene.GetCard(out second_card);
            if (card == -1)
                return;
            if (Card.IsPlus4(card))
            {
                Plus4Assigned = false;
                Plus4WillAssign = true;
            }
            if (second_card != -1)
            {
                var result = MessageBox.Show("Play 2 Same Card ?", "Make Choice", MessageBoxButton.YesNo);
                if (result == MessageBoxResult.No)
                    second_card = -1;
                else
                    last_cards.Add(second_card);
            }

            service.Play(card, second_card);
            last_cards.Add(card);
        }

        List<int> last_cards = new List<int>(2);
        private void OnPlayCardResult(bool success, List<int> cards)
        {
            if (success)
            {
                foreach (var card in last_cards)
                    hand_scene.RemoveCard(card);
            }
            else
            {
                hand_scene.OnCardsUpdated(cards.Count, cards);

            }
            last_cards.Clear();
        }

        private void buttonDraw_Click(object sender, RoutedEventArgs e)
        {
            service.Draw();
        }

        private void buttonEnd_Click(object sender, RoutedEventArgs e)
        {
            service.Done();
        }

        private void buttonUNO_Click(object sender, RoutedEventArgs e)
        {
            service.Uno();
        }

        private void SendChatMessage()
        {
            if (textboxChat.Text.Length != 0)
            {
                service.SendChatMessage(textboxChat.Text);
                textboxChat.Clear();
            }
        }

        private void Go_Click(object sender, RoutedEventArgs e)
        {
            SendChatMessage();
        }

        private void ClearChatLog(object sender, RoutedEventArgs e)
        {
            richtextboxChat.Document.Blocks.Clear();
        }

        private void groupboxChat_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            richtextboxChat.Height = groupboxChat.ActualHeight - 45;
        }

        private void d2dUser_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            int id = user_scene.GetUser();
            if (id == -1)
                return;
            if (Plus4Assigned)
            {
                service.Doubt();
                Plus4Assigned = false;
                return;
            }

            if (Plus4WillAssign)
            {
                service.ChoosePlus4User(id);
                Plus4WillAssign = false;
                return;
            }

            service.Doubt(id);
        }

        private void Red_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            service.ChooseColor(GameMsg.ColorInfo.Red);
        }

        private void Yellow_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            service.ChooseColor(GameMsg.ColorInfo.Yellow);
        }

        private void Blue_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            service.ChooseColor(GameMsg.ColorInfo.Blue);
        }

        private void Green_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            service.ChooseColor(GameMsg.ColorInfo.Green);
        }

        private void textboxChat_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SendChatMessage();
            }
        }

        private void Window_KeyUp(object sender, KeyEventArgs e)
        {
            service.Uno();
        }
    }

    public class RoomDetailEx : RoomDetail, INotifyPropertyChanged
    {
        public RoomDetailEx(RoomDetail detail)
        {
            Update(detail);
        }

        public void Update(RoomDetail detail)
        {
            this.name = detail.name;
            this.number = detail.number;
            this.password = detail.password;
            this.timeout = detail.timeout;
            this.state = detail.state;
            state_show = detail.state == RoomDetail.RoomState.Wait ? "Wait" : "Running";
            OnPropertyChanged("state_show");
            OnPropertyChanged("name");
            OnPropertyChanged("timeout");
        }

        public string state_show { get; set; }
        public event PropertyChangedEventHandler PropertyChanged;
        void OnPropertyChanged(string name)
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (handler != null)
                handler(this, new PropertyChangedEventArgs(name));
        }
    }
}
