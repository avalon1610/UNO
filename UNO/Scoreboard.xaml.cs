using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Windows;

namespace UNO
{
    /// <summary>
    /// Interaction logic for Scoreboard.xaml
    /// </summary>
    public partial class Scoreboard : Window
    {
        internal Scoreboard(List<ScoreInfo> info, UserScene scene)
        {
            InitializeComponent();
            listBoxScores.DataContext = this;
            ScoreList = new ObservableCollection<Score>();
            foreach (var i in info)
            {
                ScoreList.Add(new Score(i, scene.FindUser(i.userID)));
            }
        }

        public ObservableCollection<Score> ScoreList { get; set; }
    }

    public class Score : ScoreInfo
    {
        public string name { get; set; }
        public Score(ScoreInfo info, string name)
        {
            this.userID = info.userID;
            this.name = name;
            this.score = info.score;
        }
    }
}
