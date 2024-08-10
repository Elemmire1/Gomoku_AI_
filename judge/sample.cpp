    #include "AIController.h"
    #include <utility>
    #include <cstring>
    #include <vector>
    #include <unordered_map>
    #include <queue>



    #define ratio 1
    #define ratio2 1

    extern int ai_side; //0: black, 1: white

    std::string ai_name = "El's Robot";

    int turn = 0;
    int board[15][15];
    std::unordered_map<int,int> value;
    std::unordered_map<int,int> point_value;

    struct point{
        int first;
        int second;
        point operator+(const point& other)
        {
            point res;
            res.first=first+other.first;
            res.second=second+other.second;
            return res;
        }
        point operator-(const point& other)
        {
            point res;
            res.first=first-other.first;
            res.second=second-other.second;
            return res;
        }
        bool operator<(const point& other) const
        {
            return true;
        }
    };
    std::vector<point> dir={{1,0},{0,1},{1,1},{1,-1}};
    struct leaf{
        int score;
        int step;
        std::vector<point> steps;
    };



    int get(point p)
    {
        return board[p.first][p.second];
    }
    void put(point p ,int color)
    {
        board[p.first][p.second]=color;
    }
    void remove(point p)
    {
        board[p.first][p.second]=-1;
    }

    bool inboard(point p)
    {
        return 0<=p.first && p.first<=14 && 0<=p.second && p.second<=14;
    }

    void func1(int &sum,int a,int b,int c,int d,int e,const int &color,const int & current_color,const int &length,int index)
    {
        int flag = current_color == color ? 1 : -ratio;

        if (length == 5) sum+= value[5] * flag; //连五特判

        //特判该死的 1 1 -1 1 与 1 -1 1 1
        if(length==2&&board[a][b]==-1)
        {
            point p={a,b};
            point r1 = p + dir[index], r2 = r1 + dir[index], r3 = r2 + dir[index], l1 = p - dir[index], l2 =l1 - dir[index],l3=l2-dir[index];
            if(inboard(p+dir[index])&&get(p+dir[index])==current_color)
            {
                if(inboard(r2)&&get(r2)==current_color) //四   ?  1  1  -1  1  1  ?
                {
                    if(inboard(r3)&&get(r3)!=1-current_color&&inboard(l3)&&get(l3)!=1-current_color)
                    sum+=value[4]/2*flag; //活四
                    else if (!(  (!inboard(r3) || get(r3)==1-current_color)  &&  (!inboard(l3) ||get(l3)==1-current_color )  )  )
                        sum+=value[-4]/2*flag; //死四
                }
                else if(inboard(r2)&&get(r2)==-1 && inboard(p-dir[index]-dir[index]-dir[index]) && get(p-dir[index]-dir[index]-dir[index]) ==- 1)
                {
                    sum+=value[3]/2*flag;  //活三
                }
                if(  ! ( (!inboard(p+dir[index]+dir[index]) || get(p+dir[index]+dir[index])==1-current_color) && (!inboard(p-dir[index]-dir[index]-dir[index]) || get(p-dir[index]-dir[index]-dir[index]) == 1-current_color)))
                {
                    sum+=value[-length-1]*flag;
                }
            }
        }

        if(length==1&&board[a][b]==-1)     // l2  1  -1  1  1  r3
        {
            point p = {a, b};
            point r1 = p + dir[index], r2 = r1 + dir[index], r3 = r2 + dir[index], r4=r3+dir[index], l1 = p - dir[index], l2 =l1 - dir[index];
            if (inboard(r1) && inboard(r2) && get(r1) == current_color && get(r2) == current_color)
            {
                if(inboard(r3) && get(r3) ==  current_color) // l2  1  -1  1  1  1  r4
                {
                    if(inboard(r4)&&get(r4)!=1-current_color && inboard(l2) && get(l2) ==-1)
                    sum+=value[4]/2*flag;
                    else if (!(  (!inboard(r4) || get(r4)==1-current_color)  &&  (!inboard(l2) ||get(l2)==1-current_color )  )  )
                    sum+=value[-4]/2*flag;
                }
                else if (inboard(r3) && get(r3) != 1 - current_color && inboard(l2) && get(l2) != 1 - current_color)
                {
                    sum += value[length + 1] * flag;
                }
                if (!(  (!inboard(r3) || get(r3)==1-current_color)  &&  (!inboard(l2) ||get(l2)==1-current_color )  )  )
                {
                    sum+=value[-length-1]*flag;
                }
                }
            }

        if (board[a][b] == -1 && (e >= 0 && board[c][d] == -1))
            sum += value[length] * flag; //活
        else if (e >= 0 && board[c][d] == -1 && board[a][b] == 1 - color)
            sum += value[-length] * flag; //死
        else if (e >= 0 && board[c][d] == 1 - color && board[a][b] == -1)
            sum += value[-length] * flag; //死
        else if (e < 0 && board[a][b] == -1)
            sum += value[-length] * flag; //死
    }

    void func1_(int &sum,int a,int b,int c,const int &color,const int & current_color,const int &length)
    {
        if (length == 5) sum+= color == current_color ? value[5] : -ratio * value[5];
        int flag;
        if (current_color == color) flag = 1; else flag = -ratio;
        if (c >= 0 && board[a][b] == -1)
            sum += value[-length] * flag; //死棋
    }

    int evaluate(int color) //棋盘的评估函数
    {
        int sum = 0;

        /*
         *     -1 0 0 0 -1   活 hh
         *
         *      1 0 0 0 -1  hh
         *     -1 0 0 0 1    hh
         *      | 0 0 0 -1  hh
         *     -1 0 0 0 |    死 hh
         *
         *     1 0 0 0 1
         *     | 0 0 0 1
         *     1 0 0 0 |     彻底死
         *
         * */

        for (int i = 0; i <= 14; i++) //遍历所有列
        {
            int current_color = board[i][0];
            int length = 1;

            for (int j = 1; j <= 14; j++) {
                if (board[i][j] != current_color) {
                    if (current_color != -1) //保证不是空格相连
                    {
                            func1(sum,i,j,i,j-length-1,j-length-1,color,current_color,length,1);
                    }
                    current_color = board[i][j];
                    length = 0;
                }
                length++;
            }

            if (current_color != -1) {
                func1_(sum,i,14-length,14-length,color,current_color,length);
            }
        }
        //cout << sum << endl;
        for (int i = 0; i <= 14; i++) //遍历所有行
        {
            int current_color = board[0][i];
            int length = 1;

            for (int j = 1; j <= 14; j++) {
                if (board[j][i] != current_color) {
                    if (current_color != -1) //保证不是空格相连
                    {
                        func1(sum,j,i,j-length-1,i,j-length-1,color,current_color,length,0);
                    }
                    current_color = board[j][i];
                    length = 0;
                }
                length++;
            }

            if (current_color != -1) {
                func1_(sum,14-length,i,14-length,color,current_color,length);
            }
        }
        //cout << sum << endl;
        for (int i = 0; i <= 14; i++) //遍历所有左上到右下
        {
            int current_color = board[i][0];
            int length = 1;

            for (int j = 1; j <= 14 - i; j++) {
                if (board[i + j][j] != current_color) {
                    if (current_color != -1) //保证不是空格相连
                    {
                        func1(sum,i+j,j,i + j - length - 1,j - length - 1,j - length - 1,color,current_color,length,2);
                    }
                    current_color = board[i + j][j];
                    length = 0;
                }
                length++;
            }

            if (current_color != -1) {
                func1_(sum,i + 14 - i - length,14 - length,14 - length,color,current_color,length);
            }
        }



        //cout << sum << endl;

        for (int i = 0; i <= 14; i++) //继续左上到右下
        {
            int current_color = board[0][i];
            int length = 1;

            for (int j = 1; j <= 14 - i; j++) {
                if (board[j][i + j] != current_color) {
                    if (current_color != -1) //保证不是空格相连
                    {
                        func1(sum,j,i+j,j - length - 1,i + j - length - 1,j - length - 1,color,current_color,length,2);
                    }
                    current_color = board[j][i + j];
                    length = 0;
                }
                length++;
            }

            if (current_color != -1) {
                func1_(sum,14-i-length,i+14-i-length,14-i-length,color,current_color,length);
            }
        }

        //cout << sum << endl;

        for (int i = 0; i <= 14; i++) //遍历左下到右上
        {
            int current_color = board[i][14];
            int length = 1;

            for (int j = 1; j <= 14 - i; j++) {
                if (board[i + j][14 - j] != current_color) {
                    if (current_color != -1) //保证不是空格相连
                    {
                        func1(sum,i+j,14-j,i + j - length - 1,14 - (j - length - 1),j - length - 1,color,current_color,length,3);
                    }
                    current_color = board[i + j][14 - j];
                    length = 0;
                }
                length++;
            }

            if (current_color != -1) {
                func1_(sum,i + 14 - i - length,i + length,14 - i - length,color,current_color,length);
            }
        }
        //cout << sum << endl;
        for (int i = 0; i <= 14; i++) //继续左下到右上
        {
            int current_color = board[0][i];
            int length = 1;

            for (int j = 1; j <= i; j++) {
                if (board[j][i - j] != current_color) {
                    if (current_color != -1) //保证不是空格相连
                    {
                        func1(sum,j,i-j,j - length - 1,i - (j - length - 1),j - length - 1,color,current_color,length,3);

                    }
                    current_color = board[j][i - j];
                    length = 0;
                }
                length++;
            }

            if (current_color != -1) {
                func1_(sum,i-length,length,i-length,color,current_color,length);
            }
        }
        return sum;
    }

    bool is_win()
    {
        return evaluate(ai_side)>=80000000;
    }

    bool is_loose()
    {
        return evaluate(ai_side)<=-80000000;
    }

    int evaluate_point(point p, int ai_color)  //用于评估一个空格有多好
    {
        // 我们如下给分：（下一步若可形成） 连五 1000000分  活四 10000分  死四、活三 100分  死三 活二10分
        int sum=0;
        for(int color=0;color<=1;color++)
        {
            int flag = ai_color == color ? 1 : ratio2;

            for (int i = 0; i <= 3; i++) {
                int length = 1;
                point r_end = p + dir[i], l_end = p - dir[i];
                int flag_r = 1, flag_l = 1; //1为死端，0为活端

                while (inboard(r_end) && get(r_end) == color) {
                    length++;
                    r_end = r_end + dir[i];
                }
                if (inboard(r_end) && get(r_end) == -1) flag_r = 0;

                while (inboard(l_end) && get(l_end) == color) {
                    length++;
                    l_end = l_end - dir[i];
                }
                if (inboard(l_end) && get(l_end) == -1) flag_l = 0;

                if (length>=5) sum+=point_value[5]*flag;
                if (flag_l == 0 && flag_r == 0) sum += point_value[length] * flag; //两个活端
                else if (!flag_l * flag_r == 1) sum += point_value[-length] * flag;  //有一端封死了
            }
        }
        return sum;

    }

    std::vector<point> attack_points()  //返回一个空格，这个空格下ai_side,能够凑出活三以上的棋形；这个空格下human_side，能凑出四以上的棋形
    {
        std::vector<point> res;
        std::unordered_map<int,int> is_in;

        for(int i=0;i<=14;i++)
        {
            for(int j=0;j<=14;j++)
            {
                if(is_in[i+1000*j]==1) continue;
                point p{i,j};
                if(get(p)!=-1) continue;
                if(evaluate_point(p,ai_side)>=200) {is_in[i+1000*j]=1;res.push_back(p);}
            }
        }

        for(int i=0;i<=14;i++)
        {
            for(int j=0;j<=14;j++)
            {
                if(is_in[i+1000*j]==1) continue;
                point p{i,j};
                if(get(p)!=-1) continue;
                for (int i = 0; i <= 3; i++) {
                    int length = 1;
                    point r_end = p + dir[i], l_end = p - dir[i];
                    int flag_r = 1, flag_l = 1; //1为死端，0为活端

                    while (inboard(r_end) && get(r_end) == 1-ai_side) {
                        length++;
                        r_end = r_end + dir[i];
                    }
                    if (inboard(r_end) && get(r_end) == -1) flag_r = 0;

                    while (inboard(l_end) && get(l_end) == 1-ai_side) {
                        length++;
                        l_end = l_end - dir[i];
                    }
                    if (inboard(l_end) && get(l_end) == -1) flag_l = 0;

                    if (length == 4 && !(flag_l == 1 && flag_r == 1) || length==5) {res.push_back(p);is_in[i+1000*j]=1;}
                }
            }
        }


        return res;
    }

    bool win(point p)
    {
        if(get(p)!=ai_side) return false;
        int length = 1;
        for (int i = 0; i <= 3; i++) {

            point r_end = p + dir[i], l_end = p - dir[i];
            int flag_r = 1, flag_l = 1;

            while (inboard(r_end) && get(r_end) == ai_side) {
                length++;
                r_end = r_end + dir[i];
            }

            while (inboard(l_end) && get(l_end) == ai_side) {
                length++;
                l_end = l_end - dir[i];
            }
        }
        return length == 5;
    }

    std::vector<point> possible_points(int tot)
    {
        //我们希望在这里挑选出最有可能的棋步进行返回，比如返回得分最高的5个点
        std::vector<point> res;
        std::vector<point> five;
        std::vector<point> four;
        std::vector<point> three;
        std::vector<point> rest;
        std::priority_queue <std::pair<int,point>> rest_list;

        for(int i=0;i<=14;i++)
            for(int j=0;j<=14;j++)
            {
                if(board[i][j]!=-1) continue;
                point p={i,j};
                if(win(p)) return std::vector<point>{p};
                if(evaluate_point(p,ai_side)>=point_value[5]) five.push_back(p);
                else if(evaluate_point(p,ai_side)>=point_value[4]) four.push_back(p);
                else if(evaluate_point(p,ai_side)>=point_value[3]) three.push_back(p);
                //else if(adjacent(p.first,p.second))     //要加吗？
                else    rest_list.push({evaluate_point(p,ai_side),p});

            }
        if(!five.empty()) return five;

            while(!rest_list.empty())
            {
                point tmp=rest_list.top().second;
                rest_list.pop();
                rest.push_back(tmp);
            }

            for(int i=0;i<four.size();i++)
            {
                res.push_back(four[i]);
                tot--;
            }
            for(int i=0;i<three.size();i++)
            {
                if(tot>0)
                {
                    res.push_back(three[i]);
                    tot--;
                }
            }
            for(int i=0;i<rest.size();i++)
            {
                if(tot>0)
                {
                    res.push_back(rest[i]);
                    tot--;
                }
            }


        return res;
    }

    leaf r(int deep,int alpha,int beta, int color, int step, std::vector<point>steps,int spread)
    {
        int e= evaluate(color);
        leaf lf={e,step,steps};

        if(deep==0) return lf; //搜到头了

        leaf best={-1145141145,step,steps};

        std::vector<point> points=possible_points(turn<=4? 225:30);

        if(points.empty()) return lf;

        for(int i=0;i<points.size();i++) //开始对所有能下棋的地方进行暴搜
        {
            put(points[i],color);
            std::vector<point>new_steps=steps;
            new_steps.push_back(points[i]);
            leaf v=r(deep-1,-beta,-alpha,1-color,step+1,new_steps,spread);
            v.score*=-1;


            remove(points[i]);

            //需要剪枝

            if(v.score>best.score) best=v;

            alpha=std::max(alpha,best.score);
            if(v.score>=beta)
                return v;
        }


        return best;
    }

    leaf try_check(int deep,int alpha,int beta, int color, int step, std::vector<point>steps,int spread)
    {
        int e= evaluate(color);
        leaf lf={e,step,steps};

        if(is_loose()){lf.score=-114514; return lf;}
        if(is_win()) {lf.score=114514; return lf;}
        if(deep==0)
        {
            //lf.steps.insert(steps.begin(),point{-1,-1});
            return lf;
        }

        leaf best={-1145141145,step,steps};

        std::vector<point> points= attack_points();
/*        if(points.size()>=2)
            points.assign(points.begin(),points.begin()+1);*/

        if(points.empty())
        {
            //lf.steps.insert(steps.begin(),point{-1,-1});
            return lf;
        }

        for(int i=0;i<points.size();i++) //开始对所有能下棋的地方进行暴搜
        {
            put(points[i],color);
            std::vector<point>new_steps=steps;
            new_steps.push_back(points[i]);
            leaf v=try_check(deep-1,-beta,-alpha,1-color,step+1,new_steps,spread);
            v.score*=-1;


            remove(points[i]);
            //需要剪枝

            if(v.score>best.score) best=v;

            alpha=std::max(alpha,best.score);
            if(v.score>=beta)
                return v;
        }


        return best;
    }



    //init function is called once at the beginning
    void init() {
        /* TODO: Replace this by your code */
        value[6]=value[-6]=value[-5]=value[7]=value[-7]=10000000;
        value[5]=100000000; //活五
        value[4]=1000000;//活四
        value[3]=1000;//活三
        value[2]=100;//活二
        value[1]=1;//一个
        value[-4]=1000;//死四
        value[-3]=100;//死三
        value[-2]=1;//死二
        point_value[5]=point_value[-5]=point_value[6]=point_value[7]=1000000;
        point_value[4]=10000;
        point_value[3]=point_value[-4]=100;
        point_value[2]=point_value[-3]=10;
        point_value[0]=point_value[1]=point_value[-2]=1;
        memset(board, -1, sizeof(board));
    }

    // loc is the action of your opponent
    // Initially, loc being (-1,-1) means it's your first move
    // If this is the third step(with 2 black ), where you can use the swap rule, your output could be either (-1, -1) to indicate that you choose a swap, or a coordinate (x,y) as normal.

    std::pair<int, int> getRandom() {
        while (true) {
            int x = rand() % 15;
            int y = rand() % 15;
            if (board[x][y] == -1) {
                board[x][y] = ai_side;
                return std::make_pair(x, y);
            }
        }
    }

  void flip()
  {
        for(int i=1;i<=14;i++)
        for(int j=1;j<=14;j++)
        {
            if(board[i][j]!=-1)
                board[i][j]=1-board[i][j];
        }
        return ;

  }

    void print()
    {
        for (int i = 0; i < 15; i++)
        {
            for (int j = 0; j <= 14; j++)
            {
                if (board[i][j] == -1) std::cout << "_ ";
                else std::cout << board[i][j] << " ";
            }
            std::cout << std::endl;
        }
    }

    std::pair<int, int> action(std::pair<int, int> loc) {
        /* TODO: Replace this by your code */
        /* This is now a random strategy */
        //std::cout<<loc.first<<"  "<<loc.second<<std::endl;

        turn++;
        if(loc.first!=-1 && loc.second!=-1)
        board[loc.first][loc.second] = 1 - ai_side;   //ai_side取0或1，0先，1后， 1-ai_side就是放上一个人类的棋子

        if(loc.first==-1 && loc.second==-1 && turn!=1)
            flip();   //满足人类的换边需求

            if(turn==1 && ai_side==0)    //不起作用，为什么？
            {
                put(point{14,14},ai_side);
                //std::cout<<114514<<std::endl;
                return {14,14};
            } //第一步摆烂，防止被换边
            
            leaf move; //前四步棋，搜两步，100个点 ； 后面的，搜六步，15个点
            //先算杀
            move=try_check(10,-1145141145,1145141145,ai_side,0,std::vector<point>{},1);
            if(move.score==114514) {board[move.steps[0].first][move.steps[0].second]=ai_side;return{move.steps[0].first,move.steps[0].second};}

            if(turn<=4)
            move=r(2,-1145141145,1145141145,ai_side,0,std::vector<point>{},1);
            else  move=r(6,-1145141145,1145141145,ai_side,0,std::vector<point>{},1);

         //std::cout<<move.score<<std::endl;
        point res=move.steps[0];
        board[res.first][res.second]=ai_side;  //return {0,0};
        //print();
        return std::make_pair(res.first,res.second);

    }
    //四层前35能过
    //四层前30平局