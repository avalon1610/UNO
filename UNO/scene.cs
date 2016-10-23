using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Input;
using D2D = Microsoft.WindowsAPICodePack.DirectX.Direct2D1;
using DWrite = Microsoft.WindowsAPICodePack.DirectX.DirectWrite;

namespace UNO
{
    sealed class UserScene : MyScene
    {
        public UserScene(MainWindow window)
            : base(window)
        {

        }

        public void OnPlayStateUpdated(PlayState state)
        {
            if (!window.GameRunning)
                return;
            left_card = state.left_card;
            ResetUserState();

            if (users.ContainsKey(state.now_turn))
                users[state.now_turn].present = true;
            if (users.ContainsKey(state.next_turn))
                users[state.next_turn].next = true;
            now_card = state.now_card;
            double_card = state.double_card;
            intercepted = state.intercepted;
        }

        bool double_card = false;
        bool intercepted = false;
        int now_card = -1;
        public void ResetPlayState()
        {
            this.now_card = -1;
            this.left_card = 0;
            this.double_card = false;
            this.intercepted = false;
        }

        public void ResetUserState()
        {
            // reset all user
            foreach (var user in users.Values)
            {
                user.present = false;
                user.next = false;
            }
        }

        private bool InUserRegion(Point point, User user)
        {
            double range = Math.Sqrt(
                Math.Pow(Math.Abs(point.X - user.X), 2) +
                Math.Pow(Math.Abs(point.Y - user.Y), 2));
            return (range <= User.radius);
        }

        public int GetUser()
        {
            Point point = Mouse.GetPosition(window.d2dUser);
            foreach (var user in users)
            {
                if (InUserRegion(point, user.Value))
                {
                    return user.Key;
                }
            }
            return -1;
        }

        public string FindUser(int id)
        {
            if (id == 0)
                return "system";
            foreach (var user in users)
            {
                if (user.Key == id)
                    return user.Value.name;
            }

            return "";
        }

        int left_card = 0;
        public static Dictionary<int, D2D.Point2F> coordinates = new Dictionary<int, D2D.Point2F>();
        SortedList<int, User> users = new SortedList<int, User>();
        public void OnUserInfoUpdated(List<UserInfo> info, bool all_user_updated)
        {
            if (!all_user_updated)
            {
                if (users.ContainsKey(info[0].ID))
                    users[info[0].ID].info = info[0];
                return;
            }

            foreach (var user in info)
            {
                if (users.ContainsKey(user.ID))
                {
                    users[user.ID].info = user;
                    users[user.ID].updated = true;
                }
                else
                {
                    lock (users)
                    {
                        users.Add(user.ID, new User(user));
                    }
                }
            }

            // remove user who not exist
            List<int> need_remove = new List<int>();
            foreach (var user in users)
            {
                if (!user.Value.updated)
                {
                    need_remove.Add(user.Key);
                    continue;
                }
                user.Value.updated = false; // reset   
            }

            lock (users)
            {
                foreach (var r in need_remove)
                {
                    users.Remove(r);
                }
            }

            // update position
            // my id is always postion 0
            if (users.Count > 8)
                return;
            Queue<int> sequence = new Queue<int>();
            int step = 1;
            int pos = 0;
            users[window.my_id].position = pos;
            if (users.Count <= 4)
                step = 2;
            foreach (var id in users.Keys)
            {
                if (id < window.my_id)
                    sequence.Enqueue(id);
                else if (id == window.my_id)
                    continue;
                else
                {
                    pos += step;
                    users[id].position = pos;
                }
            }

            while (sequence.Count != 0)
            {
                pos += step;
                users[sequence.Dequeue()].position = pos;
            }
        }

        void UpdateSeatPosition(float x, float y, float r)
        {
            float l = (float)(r / Math.Sqrt(2));
            coordinates[0] = new D2D.Point2F(x, y + r);
            coordinates[1] = new D2D.Point2F(x - l, y + l);
            coordinates[2] = new D2D.Point2F(x - r, y);
            coordinates[3] = new D2D.Point2F(x - l, y - l);
            coordinates[4] = new D2D.Point2F(x, y - r);
            coordinates[5] = new D2D.Point2F(x + l, y - l);
            coordinates[6] = new D2D.Point2F(x + r, y);
            coordinates[7] = new D2D.Point2F(x + l, y + l);
        }


        protected override void OnRender()
        {
            // draw a table
            var size = RenderTarget.Size;
            float table_radius = Math.Min(size.Width, size.Height) / 2 - User.radius;
            float table_x = size.Width / 2;
            float table_y = size.Height / 2;
            if (RenderSize != size)
            {
                RenderSize = size;
                UpdateSeatPosition(table_x, table_y, table_radius);
            }

            var show_leftcard = string.Format("Left: {0} Card{1}", left_card, left_card < 2 ? "" : "s");
            int now_color = -1, now_number = -1;
            Card now_card = null;
            if (this.now_card != -1 && Card.GetCard(this.now_card, ref now_color, ref now_number))
            {
                now_card = new Card(now_color, now_number);
                now_card.X = table_x - Card.width / 2;
                now_card.Y = table_y - Card.height / 2;
            }
            RenderTarget.BeginDraw();
            RenderTarget.Clear(backgroud);
            RenderTarget.DrawText(show_leftcard, textFormat, new D2D.RectF(10, 20, 100, 20), blackBrush);
            RenderTarget.DrawUser(users, ratio, blueBrush, textFormat);
            if (now_card != null)
            {
                now_card.NeedRedraw = true;
                RenderTarget.DrawCard(now_card, redBrush, yellowBrush, greenBrush,
                    blueBrush, blackBrush, whiteBrush, numberFormat, functionFormat);
                if (double_card)
                {
                    float double_x = now_card.X + Card.width + 10;
                    float double_y = now_card.Y + Card.height - 10;
                    var rect = new D2D.RectF(double_x, double_y, double_x + 20, double_y + 10);
                    RenderTarget.DrawText("x2", textFormat, rect, blackBrush);
                }

                if (intercepted)
                {
                    float inter_x = now_card.X + Card.width / 2;
                    float inter_y = now_card.Y + Card.height + 10;
                    var rect = new D2D.RectF(inter_x, inter_y, inter_x + 20, inter_y + 20);
                    RenderTarget.DrawText("抢", functionFormat, rect, redBrush);
                }
            }
            base.OnRender();
            RenderTarget.EndDraw();
        }
    }

    class User
    {
        public static float radius = 40.0f;
        public UserInfo info { get; set; }
        public bool updated { get; set; }
        public bool present { set; private get; }
        public bool next { set; private get; }
        public double ratio { private get; set; }
        public string name { get; private set; }
        public D2D.Ellipse ellipse
        {
            get
            {
                double range = 0;
                if (present) range = 10;
                var point = UserScene.coordinates[position];
                if (next)
                    point.Y -= (float)(range * ratio);
                float r = (float)(radius - range * ratio);
                return new D2D.Ellipse(point, r, r);
            }
        }

        public float X { get { return UserScene.coordinates[position].X; } }
        public float Y { get { return UserScene.coordinates[position].Y; } }
        public D2D.RectF ContentLayout
        {
            get
            {
                var point = UserScene.coordinates[position];
                float x = point.X - (float)(radius / Math.Sqrt(2));
                float y = point.Y - (float)(radius / Math.Sqrt(2));
                float l = (float)(radius * Math.Sqrt(2));
                return new D2D.RectF(x, y, x + l, y + l);
            }
        }
        public string Content
        {
            get
            {
                string state = "";
                switch (info.state)
                {
                    case UserInfo.State.idle:
                        state = "idle";
                        break;
                    case UserInfo.State.ready:
                        state = "ready";
                        break;
                    case UserInfo.State.uno:
                        state = "uno";
                        break;
                }
                string content = string.Format("ID:{0}\nname:{1}\nstate:{2}\ncards:{3}",
                    info.ID, info.name, state, info.card_count);
                return content;
            }
        }

        public int position { get; set; }

        public User(UserInfo info)
        {
            this.info = info;
            updated = true;
            ratio = 1;
            present = false;
            next = false;
            name = info.name;
        }
    }

    abstract class MyScene : Direct2D.AnimatedScene
    {
        protected DWrite.DWriteFactory writeFactory;
        protected D2D.SolidColorBrush redBrush, whiteBrush, yellowBrush, blueBrush, greenBrush, blackBrush;
        protected DWrite.TextFormat textFormat, numberFormat, functionFormat;

        protected D2D.ColorF backgroud = new D2D.ColorF(1, 1, 1, 0.5f);
        // These are used for tracking an accurate frames per second
        private DateTime time;
        private int frameCount;
        private int fps;
        protected MainWindow window;
        static protected double ratio;
        static protected bool increment = true;
        public MyScene(MainWindow window)
            : base(100) // Will probably only be about 67 fps due to the limitations of the timer
        {
            this.window = window;
            this.writeFactory = DWrite.DWriteFactory.CreateFactory();
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
                this.writeFactory.Dispose();
            base.Dispose(disposing);
        }
        protected D2D.SizeF RenderSize;
        protected override void OnCreateResources()
        {
            // We don't need to free any resources because the base class will
            // call OnFreeResources if necessary before calling thie method
            redBrush = RenderTarget.CreateSolidColorBrush(new D2D.ColorF(1, 0, 0));
            yellowBrush = RenderTarget.CreateSolidColorBrush(new D2D.ColorF(1, 0.78f, 0));
            whiteBrush = RenderTarget.CreateSolidColorBrush(new D2D.ColorF(1, 1, 1));
            greenBrush = RenderTarget.CreateSolidColorBrush(new D2D.ColorF(0.32f, 1, 0.32f));
            blueBrush = RenderTarget.CreateSolidColorBrush(new D2D.ColorF(0, 0, 1));
            blackBrush = RenderTarget.CreateSolidColorBrush(new D2D.ColorF(0, 0, 0));
            textFormat = writeFactory.CreateTextFormat("Arial", 12);
            textFormat.TextAlignment = DWrite.TextAlignment.Center;
            textFormat.WordWrapping = DWrite.WordWrapping.NoWrap;
            numberFormat = writeFactory.CreateTextFormat("Microsoft YaHei UI", 60);
            numberFormat.TextAlignment = DWrite.TextAlignment.Center;
            functionFormat = writeFactory.CreateTextFormat("Microsoft YaHei UI", 40);
            functionFormat.TextAlignment = DWrite.TextAlignment.Trailing;

            base.OnCreateResources();  // Call this last to start the animation
        }

        protected override void OnFreeResources()
        {
            base.OnFreeResources(); // Call this first to stop the animation
            if (redBrush != null)
            {
                redBrush.Dispose();
                redBrush = null;
            }

            if (whiteBrush != null)
            {
                whiteBrush.Dispose();
                whiteBrush = null;
            }

            if (yellowBrush != null)
            {
                yellowBrush.Dispose();
                yellowBrush = null;
            }

            if (greenBrush != null)
            {
                greenBrush.Dispose();
                greenBrush = null;
            }

            if (blueBrush != null)
            {
                blueBrush.Dispose();
                blueBrush = null;
            }

            if (blackBrush != null)
            {
                blackBrush.Dispose();
                blackBrush = null;
            }

            if (textFormat != null)
            {
                textFormat.Dispose();
                textFormat = null;
            }

            if (numberFormat != null)
            {
                numberFormat.Dispose();
                numberFormat = null;
            }

            if (functionFormat != null)
            {
                functionFormat.Dispose();
                functionFormat = null;
            }
        }

        protected override void OnRender()
        {
            // Calculate our actual frame rate
            frameCount++;
            if (DateTime.UtcNow.Subtract(time).TotalSeconds >= 1)
            {
                fps = frameCount;
                frameCount = 0;
                time = DateTime.UtcNow;
            }

            // Draw a little FPS in the top left corner
            string text = string.Format("FPS {0}", fps);
            RenderTarget.DrawText(text, textFormat, new D2D.RectF(10, 10, 100, 20), blackBrush);
        }
    }

    sealed class HandScene : MyScene
    {
        public HandScene(MainWindow window)
            : base(window)
        {

        }

        public void OnDoubtCardInfo(int count, List<int> cards)
        {
            if (count != cards.Count)
                return;

            TemporaryCardList.Clear();
            int color_index = -1, number_index = -1;
            foreach (var card in cards)
            {
                if (Card.GetCard(card, ref color_index, ref number_index))
                {
                    var new_card = new Card(color_index, number_index);
                    TemporaryCardList.Add(card, new_card);
                    new_card.NeedRedraw = true;
                }
            }
            NeedClear = true;
        }

        SortedList<int, Card> TemporaryCardList = new SortedList<int, Card>();
        public void OnCardsUpdated(int count, List<int> cards)
        {
            if (count != cards.Count)
                return;

            int color_index = -1;
            int number_index = -1;
            int sameas = -1;
            foreach (var card in cards)
            {
                sameas = -1;
                if (Card.GetCard(card, ref color_index, ref number_index))
                {
                    // found same
                    foreach (var old in CardList)
                    {
                        if (Card.IsSame(old.Key, card))
                        {
                            old.Value.sameas = card;
                            sameas = old.Key;
                            break;
                        }
                    }

                    lock (CardList)
                    {
                        var new_card = new Card(color_index, number_index, sameas);
                        new_card.New = true;
                        CardList.Add(card, new_card);
                    }
                }
            }

            RedrawAllCard();
        }

        public void ResetHand()
        {
            lock (CardList)
            {
                CardList.Clear();
            }

            NeedClear = true;
        }

        bool NeedClear = false;
        SortedList<int, Card> CardList = new SortedList<int, Card>();
        protected override void OnRender()
        {
            ratio = increment ? ratio + ElapsedTime : ratio - ElapsedTime;
            if (ratio < 0)
            {
                ratio = 0;
                increment = true;
            }
            if (ratio > 1)
            {
                ratio = 1;
                increment = false;
            }

            if (RenderSize != RenderTarget.Size)
            {
                RenderSize = RenderTarget.Size;
                foreach (var card in CardList.Values)
                {
                    card.NeedRedraw = true;
                }
            }

            Point point = Mouse.GetPosition(window.d2dHand);
            lock (CardList)
            {
                if (CardList.Count != 0 && (!CardList.ContainsKey(AcitveCard) || (AcitveCard != -1 &&
                 !InCardRegion(point, CardList[AcitveCard])) || AcitveCard == -1))
                {
                    bool found = false;
                    foreach (var card in CardList)
                    {
                        if (InCardRegion(point, card.Value))
                        {
                            card.Value.Actived = true;
                            card.Value.NeedRedraw = true;
                            AcitveCard = card.Key;
                            found = true;
                            //DebugPrintCard("now active card: ", card.Value);
                        }
                        else
                            card.Value.Actived = false;
                    }

                    if (!found)
                        AcitveCard = -1;
                }
            }

            RenderTarget.BeginDraw();
            if (NeedClear)
                RenderTarget.Clear(backgroud);
            SortedList<int, Card> NowCards;
            if (TemporaryCardList.Count != 0)
                NowCards = TemporaryCardList;
            else
                NowCards = CardList;
            RenderTarget.DrawCard(NowCards, redBrush, yellowBrush, greenBrush, blueBrush,
                blackBrush, whiteBrush, numberFormat, functionFormat, ratio);
            RenderTarget.EndDraw();
            NeedClear = false;
        }
        int AcitveCard = -1;

        void DebugPrintCard(string prefix, Card card)
        {
            string color = "";
            switch (card.Color)
            {
                case 0:
                    color = "red";
                    break;
                case 1:
                    color = "blue";
                    break;
                case 2:
                    color = "green";
                    break;
                case 3:
                    color = "yellow";
                    break;
                case 4:
                    color = "black";
                    break;
            }
            Console.WriteLine("{0} {1} {2}", prefix, color, card.Content);
        }

        bool InCardRegion(Point point, Card card)
        {
            if (point.X >= card.X && point.X <= card.X + Card.width &&
                point.Y >= card.Y && point.Y <= card.Y + Card.height)
                return true;
            else
                return false;
        }

        private void RedrawAllCard()
        {
            foreach (var card in CardList.Values)
            {
                card.NeedRedraw = true;
            }

            NeedClear = true;
        }

        public int GetCard(out int second)
        {
            second = -1;
            if (TemporaryCardList.Count != 0)
            {
                TemporaryCardList.Clear();
                RedrawAllCard();
                return -1;
            }

            lock (CardList)
            {
                if (CardList.ContainsKey(AcitveCard))
                {
                    if (CardList[AcitveCard].sameas != -1)
                        second = CardList[AcitveCard].sameas;
                    //DebugPrintCard("get card return: ", CardList[AcitveCard]);
                    return AcitveCard;
                }
            }

            return -1;
        }

        public void RemoveCard(int card)
        {
            int index = -1;
            foreach (var element in CardList)
            {
                if (element.Key == card)
                    index = element.Key;
                element.Value.NeedRedraw = true;
            }

            if (index == -1)
                return;

            lock (CardList)
            {
                CardList.Remove(index);
            }

            RedrawAllCard();
        }
    }

    public static class RenderTargetExtension
    {
        internal static void DrawCard(this D2D.RenderTarget self, Card card,
            D2D.SolidColorBrush redBrush, D2D.SolidColorBrush yellowBrush, D2D.SolidColorBrush greenBrush,
            D2D.SolidColorBrush blueBrush, D2D.SolidColorBrush blackBrush, D2D.SolidColorBrush whiteBrush,
            DWrite.TextFormat numberFormat, DWrite.TextFormat functionFormat)
        {
            if (!card.NeedRedraw)
                return;
            D2D.SolidColorBrush brush = whiteBrush;
            switch (card.Color)
            {
                case 0:
                    brush = redBrush;
                    break;
                case 1:
                    brush = blueBrush;
                    break;
                case 2:
                    brush = greenBrush;
                    break;
                case 3:
                    brush = yellowBrush;
                    break;
                case 4:
                    brush = blackBrush;
                    break;
            }
            self.FillRoundedRectangle(card.Rect, brush);
            self.FillEllipse(card.Circle, whiteBrush);
            DWrite.TextFormat format = numberFormat;
            if (card.Content.Length > 1)
                format = functionFormat;
            self.DrawText(card.Content, format, card.ContentLayout, brush);
            if (card.New)
            {
                self.FillEllipse(card.NewTag, whiteBrush);
                card.New = false;
            }
            card.NeedRedraw = false;
        }

        internal static void DrawCard(this D2D.RenderTarget self, SortedList<int, Card> cards,
            D2D.SolidColorBrush redBrush, D2D.SolidColorBrush yellowBrush, D2D.SolidColorBrush greenBrush,
            D2D.SolidColorBrush blueBrush, D2D.SolidColorBrush blackBrush, D2D.SolidColorBrush whiteBrush,
            DWrite.TextFormat numberFormat, DWrite.TextFormat functionFormat, double ratio)
        {
            float with = self.Size.Width;
            float step = (with - Card.width) / (cards.Count - 1);
            float pos = 0;
            int active_card = -1;
            lock (cards)
            {
                foreach (var element in cards)
                {
                    Card card = element.Value;
                    card.ratio = ratio;
                    card.Y = 0;
                    card.X = pos;
                    pos += step;
                    if (card.Actived)
                    {
                        active_card = element.Key;
                        continue;
                    }
                    self.DrawCard(card, redBrush, yellowBrush, greenBrush, blueBrush,
                        blackBrush, whiteBrush, numberFormat, functionFormat);
                }
            }

            if (active_card != -1)
                self.DrawCard(cards[active_card], redBrush, yellowBrush, greenBrush,
                    blueBrush, blackBrush, whiteBrush, numberFormat, functionFormat);
        }

        internal static void DrawUser(this D2D.RenderTarget self, SortedList<int, User> users,
            double ratio, D2D.SolidColorBrush brush, DWrite.TextFormat format)
        {
            lock (users)
            {
                foreach (var user in users.Values)
                {
                    user.ratio = ratio;
                    self.DrawEllipse(user.ellipse, brush, 1.0f);
                    self.DrawText(user.Content, format, user.ContentLayout, brush);
                }
            }

        }
    }

    class Card
    {
        public static float width = 90;
        public static float height = 116;
        private float x;
        private float y;
        private string content;
        public int sameas { get; set; }
        public bool Actived { get; set; }
        public int Color { get; private set; }
        public bool NeedRedraw { get; set; }
        public bool New { get; set; }
        public double ratio
        {
            set
            {
                x = x + (float)(30 - 30 / value);
            }
        }

        public Card(int color, int number_index, int sameas = -1)
        {
            Color = color;
            if (number_index > 9)
            {
                string func = CardFunction[number_index - 10];
                if (func == "Any")
                    content = "";
                else
                    content = func.Substring(0, 2);
            }
            else
                content = Convert.ToString(number_index);

            this.sameas = sameas;
        }

        public static string[] CardFunction = { "Reverse", "Skip", "+2", "Change", "+4", "Any" };

        [DllImport("Card.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool GetCard(int index, ref int color, ref int number);

        [DllImport("Card.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool IsPlus4(int index);

        [DllImport("Card.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool IsSame(int _1, int _2);

        public D2D.RectF ContentLayout
        {
            get
            {
                float half_width = (width - 10) / 2;
                float half_height = (height - 10) / 2;
                float X = this.x + 5 + half_width - half_width / (float)Math.Sqrt(2);
                float Y = this.y + 5 + half_height - half_height / (float)Math.Sqrt(2);
                float W = X + half_width / (float)Math.Sqrt(2) * 2;
                float H = Y + half_height / (float)Math.Sqrt(2) * 2;
                if (content.Length > 1)
                {
                    Y += 10;
                }
                return new D2D.RectF(X, Y, W, H);
            }
        }
        public string Content { get { return content; } }
        public float X { get { return x; } set { this.x = value; } }
        public float Y { get { return y; } set { this.y = value; } }
        public D2D.Ellipse Circle
        {
            get
            {
                return new D2D.Ellipse(new D2D.Point2F(x + width / 2, y + height / 2),
                    (width - 10) / 2, (height - 10) / 2);
            }

        }

        public D2D.Ellipse NewTag
        {
            get
            {
                return new D2D.Ellipse(new D2D.Point2F(x + width / 10, y + height / 10), 8, 8);
            }
        }

        public D2D.RoundedRect Rect
        {
            get { return new D2D.RoundedRect(new D2D.RectF(x, y, x + width, y + height), 2.0f, 2.0f); }
        }
    }
}
