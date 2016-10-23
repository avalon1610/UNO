using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Windows;
using ProtoBuf;

namespace UNO
{
    delegate void RoomListResultHandler(List<RoomInfo> result);
    delegate void LoginResultHandler(LoginResult result);
    delegate void UserStatusHanlder(List<UserInfo> info, bool allUser);
    delegate void RoomStatusHanlder(RoomDetail info);
    delegate void PlayStatusHanlder(PlayState info);
    delegate void DrawCardResultHandler(int count, List<int> cards);
    delegate void PlayCardResultHandler(bool success, List<int> cards = null);
    delegate void ChatMessageHandler(long time, int id, string content);
    delegate void DoubtCardsHandler(int count, List<int> cards);
    delegate void Plus4AssingHandler(int id);
    delegate void ScoreResultHandler(List<ScoreInfo> info);
    class Service
    {
        Network net;
        long sequence = 0;
        public IniFile ini = null;
        public Service()
        {
            ini = new IniFile("Setting.ini");
            Reset();
        }

        private long generateSequence()
        {
            Random r = new Random();
            return r.Next(int.MaxValue) + sequence++;
        }

        internal void Shutdown()
        {
            if (net != null)
            {
                if (net.connected)
                    net.Dispose();
            }
            firstTime = true;
        }

        public void Reset()
        {
            Shutdown();
            net = new Network(this);
        }

        private void SendRequest<T>(T imsg, UNOMsg.Type type)
        {
            UNOMsg msg = new UNOMsg();
            msg.type = type;
            switch (type)
            {
                case UNOMsg.Type.Room:
                    msg.room_msg = imsg as RoomMsg;
                    break;
                case UNOMsg.Type.Game:
                    msg.game_msg = imsg as GameMsg;
                    break;
                case UNOMsg.Type.Chat:
                    msg.chat_msg = imsg as ChatMsg;
                    break;
                default:
                    return;
            }
            msg.sequence = generateSequence();
            using (var stream = new MemoryStream())
            {
                Serializer.SerializeWithLengthPrefix(stream, msg, PrefixStyle.Base128);
                net.Send(stream.GetBuffer());
            }
        }

        public void GetRoomList()
        {
            RoomMsg rmsg = new RoomMsg();
            rmsg.type = RoomMsg.Type.GetList;
            SendRequest(rmsg, UNOMsg.Type.Room);
        }

        public void CreateRoom(string name)
        {
            if (name.Length == 0)
                return;
            RoomMsg rmsg = new RoomMsg();
            rmsg.type = RoomMsg.Type.Create;
            LoginInfo info = new LoginInfo();
            info.user = name;
            info.room_number = 0;
            rmsg.login_info = info;
            SendRequest(rmsg, UNOMsg.Type.Room);
        }

        public void ModifyRoom(RoomDetail detail)
        {
            RoomMsg rmsg = new RoomMsg();
            rmsg.type = RoomMsg.Type.Setting;
            rmsg.room_detail = detail;
            SendRequest(rmsg, UNOMsg.Type.Room);
        }

        public void Login(int room_id, string name, string password = null)
        {
            if (name.Length == 0)
                return;
            if (room_id < 0)
                return;
            RoomMsg rmsg = new RoomMsg();
            rmsg.type = RoomMsg.Type.Login;
            LoginInfo info = new LoginInfo();
            info.user = name;
            if (password != null)
                info.password = password;
            info.room_number = room_id;
            rmsg.login_info = info;
            SendRequest(rmsg, UNOMsg.Type.Room);
        }

        public void Ready()
        {
            SendGameReqest(GameMsg.Type.Ready);
        }

        public void Play(int index, int second = -1)
        {
            GameMsg gmsg = new GameMsg();
            gmsg.type = GameMsg.Type.PlayCard;
            CardInfo card = new CardInfo();
            card.number.Add(index);
            card.count = 1;
            if (second != -1)
            {
                card.number.Add(second);
                card.count = 2;
            }
            gmsg.card_info = card;
            SendRequest(gmsg, UNOMsg.Type.Game);
        }

        public void Draw()
        {
            SendGameReqest(GameMsg.Type.DrawCard);
        }

        public void Uno()
        {
            SendGameReqest(GameMsg.Type.UNO);
        }

        public void Done()
        {
            SendGameReqest(GameMsg.Type.Done);
        }

        public void Doubt(int id = -1)
        {
            GameMsg gmsg = new GameMsg();
            gmsg.type = GameMsg.Type.Doubt;
            if (id != -1)
            {
                DoubtInfo doubt = new DoubtInfo();
                doubt.user_id = id;
                gmsg.doubt_info = doubt;
            }
            SendRequest(gmsg, UNOMsg.Type.Game);
        }

        void SendGameReqest(GameMsg.Type type)
        {
            GameMsg gmsg = new GameMsg();
            gmsg.type = type;
            SendRequest(gmsg, UNOMsg.Type.Game);
        }

        public void ChooseColor(GameMsg.ColorInfo color)
        {
            GameMsg gmsg = new GameMsg();
            gmsg.type = GameMsg.Type.Black;
            gmsg.color_info = color;
            SendRequest(gmsg, UNOMsg.Type.Game);
        }

        public void ChoosePlus4User(int id)
        {
            GameMsg gmsg = new GameMsg();
            gmsg.type = GameMsg.Type.Black;
            DoubtInfo assign = new DoubtInfo();
            assign.user_id = id;
            gmsg.doubt_info = assign;
            SendRequest(gmsg, UNOMsg.Type.Game);
        }

        public void SendChatMessage(string content)
        {
            ChatMsg cmsg = new ChatMsg();
            cmsg.content = content;
            cmsg.userID = 0;    // client don't care
            cmsg.time = 0;      // client don't care
            SendRequest(cmsg, UNOMsg.Type.Chat);
        }

        public Semaphore WaitGameWindow = new Semaphore(0, 1);
        public event RoomListResultHandler rlr_event;
        public event LoginResultHandler lr_event;
        public event UserStatusHanlder user_status_event;
        public event RoomStatusHanlder room_status_event;
        public event PlayStatusHanlder play_status_event;
        public event DrawCardResultHandler dcr_event;
        public event PlayCardResultHandler pcr_event;
        public event ChatMessageHandler chat_event;
        public event DoubtCardsHandler dc_event;
        public event Plus4AssingHandler p4a_event;
        public event ScoreResultHandler sr_event;
        bool firstTime = true;
        public void Process(byte[] buffer)
        {
            UNOMsg msg;
            int prefix_length = 0;
            byte[] left_buffer = null;
            if (Serializer.TryReadLengthPrefix(buffer, 0, buffer.Length, PrefixStyle.Base128, out prefix_length))
            {
                int step = 1;
                if (prefix_length >= 0x100000)
                    step = 3;
                else if (prefix_length >= 0x80)
                    step = 2;
                if (buffer.Length > prefix_length + step)
                {
                    int left_length = buffer.Length - prefix_length - step;
                    left_buffer = new byte[left_length];
                    Buffer.BlockCopy(buffer, prefix_length + step, left_buffer, 0, left_length);
                }
            }

            using (var stream = new MemoryStream(buffer))
            {
                msg = Serializer.DeserializeWithLengthPrefix<UNOMsg>(stream, PrefixStyle.Base128);
            }

            switch (msg.type)
            {
                case UNOMsg.Type.Room:
                    RoomMsg rmsg = msg.room_msg;
                    if (rmsg.type == RoomMsg.Type.ListResult && rlr_event != null)
                        rlr_event(rmsg.room_info);
                    else if (rmsg.type == RoomMsg.Type.LoginResult && lr_event != null)
                        lr_event(rmsg.login_result);
                    break;
                case UNOMsg.Type.Game:
                    if (firstTime)
                    {
                        WaitGameWindow.WaitOne();
                        firstTime = false;
                    }
                    GameMsg gmsg = msg.game_msg;
                    if (gmsg.type == GameMsg.Type.Status)
                    {
                        StatusInfo status = gmsg.status_info;
                        if (status == null)
                            return;
                        if (status.type == StatusInfo.StatusType.User && user_status_event != null)
                            user_status_event(gmsg.status_info.user_info, gmsg.status_info.all_user_updated);
                        else if (status.type == StatusInfo.StatusType.Room && room_status_event != null)
                            room_status_event(gmsg.status_info.room_detail);
                        else if (status.type == StatusInfo.StatusType.Play && play_status_event != null)
                            play_status_event(gmsg.status_info.play_state);
                    }
                    else if (gmsg.type == GameMsg.Type.DrawCardResult)
                    {
                        CardInfo cards = gmsg.card_info;
                        if (cards == null)
                            return;
                        if (dcr_event != null)
                            dcr_event(cards.count, cards.number);
                    }
                    else if (gmsg.type == GameMsg.Type.PlayCardResult)
                    {
                        if (pcr_event != null)
                        {
                            CardInfo cards = gmsg.card_info;
                            if (cards == null)
                                pcr_event(true);
                            else
                                pcr_event(false, cards.number);
                        }
                    }
                    else if (gmsg.type == GameMsg.Type.Black)
                    {
                        DoubtInfo doubt = gmsg.doubt_info;
                        if (doubt != null)
                        {
                            // +4 target info
                            if (p4a_event != null)
                                p4a_event(doubt.user_id);
                            return;
                        }

                        CardInfo card = gmsg.card_info;
                        if (card != null)
                        {
                            if (dc_event != null)
                                dc_event(card.count, card.number);
                            return;
                        }

                        // never goes here
                        throw new Exception("game msg black has neither doubt info nor color info");
                    }
                    break;
                case UNOMsg.Type.Chat:
                    if (chat_event != null)
                    {
                        ChatMsg cm = msg.chat_msg;
                        if (cm == null)
                            return;
                        chat_event(cm.time, cm.userID, cm.content);
                    }
                    break;
                case UNOMsg.Type.Error:
                    string err = "[Server Error] " + msg.error_msg;
                    MessageBox.Show(err);
                    break;
                case UNOMsg.Type.Score:
                    if (sr_event != null)
                    {
                        ScoreMsg smsg = msg.score_msg;
                        if (smsg == null)
                            return;
                        sr_event(smsg.score_info);
                    }
                    break;
                default:
                    break;
            }

            if (left_buffer != null)
                Process(left_buffer);
        }
    }
}
