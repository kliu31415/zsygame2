bool cmpVal(int a, int b)
{
    return value(a) < value(b);
}
trick_info lastTrick, bestTrick[MAX_PLAYERS]; //has info about the last trick played and the best trick the AI current thinks it can play
vector<int> trickPlayed;
double best_rating[MAX_PLAYERS];
struct hand
{
    int cnt[CARD_TYPES];
    vector<int> cards;
    constexpr static double MIN_RATING = -1e9;
    int playerNum, cardsInHand;
    double prv_rating; //rating sum from tricks previously found in the dfs
    double winFactor; //determines how much the AI wants to win/bomb this trick
    hand()
    {
        fill(cnt, cnt+CARD_TYPES, 0);
    }
    hand(int pnum)
    {
        playerNum = pnum;
        cardsInHand = 27;
        fill(cnt, cnt+CARD_TYPES, 0);
    }
    hand(int p, int c[CARD_TYPES], int r)
    {
        playerNum = p;
        prv_rating = r;
        copy(c, c+CARD_TYPES, cnt);
    }
    hand(vector<int> c)
    {
        fill(cnt, cnt+CARD_TYPES, 0);
        for(auto &i: c)
            cnt[i]++;
    }
    int getNumCards()
    {
        if(playerNum == PLAYER_NUM) //the player's cards are stored in a vector
            return cards.size();
        return cardsInHand; //this is updated every time the getMove() function is called
    }
    void goNext(vector<int> &trick, double trickRating, int pos1, int num, int sz, bool played)
    {
        hand nxt(playerNum, cnt, prv_rating);
        for(auto &i: trick)
            nxt.cnt[i]--;
        if(played) //we can't/don't want to play a trick
        {
            trickRating += (1 - 2*randf()) * AI_Randomness; //user changeable setting
            nxt.prv_rating += trickRating;
            nxt.createTricks(pos1, num, sz);
        }
        else //we will play this as a trick
        {
            trickPlayed = trick;
            nxt.createTricks();
        }
    }
    double straightFlushValue(int num, int sz)
    {
        return AI_StraightFlushAffinity + 40 + (26 + (sz-5)*26 + value(num+1) + getNumCards()) / 2.0; //the AI will be more hesitant to play bombs when it has more cards in hand
    }
    double bombValue(int num, int sz)
    {
        double trickRating = AI_BombAffinity + 40 + ((sz-4)*13 + value(num) + getNumCards()) / 2.0;
        if(sz >= 6) //because there are no 4 long straight flushes so 4 long bombs and 5 long bombs are effectively consecutive
            trickRating += (13*(sz-5)) / 2.0;
        return trickRating;
    }
    double straightValue(int num, int sz)
    {
        return AI_StraightAffinity + value((num+1)%13) - 30 + getNumCards(); //the AI will not like straights as much when it has less cards
    }
    double tripleValue(int num, int sz)
    {
        if(sz == 1)
            return AI_TripleAffinity + value(num) - 20;
        else return AI_TripleAffinity + value((num+1)%13) - 20;
    }
    double fullHouseValue(int num, int sz)
    {
        if(sz == 1)
            return AI_FullhouseAffinity + value(num) - 20;
        else return AI_FullhouseAffinity + value((num+1)%13) - 20;
    }
    double doubleValue(int num, int sz)
    {
        if(sz == 1)
            return AI_DoubleAffinity + value(num) - 20;
        else return AI_DoubleAffinity + value((num+1)%13) - 20;
    }
    double singleValue(int num)
    {
        return AI_SingleAffinity + value(num) - 20;
    }
    bool createStraightFlush(int num, int sz, bool played) //2s can be "demoted"
    {
        if(num%13==0 || (num%13)+sz>=15)
            return false;
        bool hasAll = true;
        vector<int> trick;
        for(int i=num; i<num+sz; i++)
        {
            int v = i;
            if(v%13 == 0) //we're checking an ace
                v -= 13;
            if(cnt[v] == 0) //we're missing part of our straight flush
            {
                hasAll = false;
            }
            else trick.push_back(v);
        }
        if(hasAll)
        {
            double trickRating = straightFlushValue(num, sz);
            goNext(trick, trickRating, 0, num, sz, played); //num+1 in case a 2 is demoted)
            return true;
        }
        return false;
    }
    bool createBomb(int num, int sz, bool played)
    {
        int sum = 0;
        for(int j=0; j<4; j++)
            sum += cnt[num + j*13];
        if(sum >= sz) //there's a bomb!
        {
            vector<int> trick;
            for(int j=0; j<4; j++)
            {
                for(int k=0; trick.size()<sz && k<cnt[num + j*13]; k++)
                {
                    trick.push_back(num + j*13);
                }
            }
            double trickRating = bombValue(num, sz);
            goNext(trick, trickRating, 1, num, sz, played);
            return true;
        }
        return false;
    }
    bool createStraight(int num, int sz, bool played)
    {
        if((num==13 && sz>1) || num%13==0 || (num%13)+sz>=15) //Aces and 2s aren't consecutive, so num==0 && sz>1 is bad
            return false;
        bool hasAll = true;
        vector<int> trick;
        for(int i=num; i<num+sz; i++)
        {
            int v = i;
            if(v%13 == 0) //we're checking an ace
                v -= 13;
            bool has = false;
            for(int j=0; j<4; j++)
            {
                if(cnt[j*13 + v])
                {
                    has = true;
                    trick.push_back(j*13 + v);
                    break;
                }
            }
            if(!has) //we're missing part of our straight
            {
                hasAll = false;
                break;
            }
        }
        if(hasAll)
        {
            double trickRating = (num+1)%13 - 30; //num+1 in case a 2 is demoted
            goNext(trick, trickRating, 2, num, sz, played);
            return true;
        }
        return false;
    }
    bool createDouble(int num, int sz, bool played)
    {
        if((num==13 && sz>1) || num>=52 || (num%13)+sz>=15) //Aces and 2s aren't consecutive, so num==13 && sz>1 is bad
            return false;
        bool hasAll = true;
        vector<int> trick;
        for(int i=num; i<num+sz; i++)
        {
            int v = i;
            if(v%13 == 0) //we're checking an ace
                v -= 13;
            int has = 0;
            for(int j=0; j<4 && has<2; j++)
            {
                for(int k=0; k<cnt[j*13 + v] && has<2; k++)
                {
                    has++;
                    trick.push_back(j*13 + v);
                }
            }
            if(has < 2) //we're missing part of our (possibly consecutive) double
            {
                hasAll = false;
                break;
            }
        }
        if(hasAll)
        {
            double trickRating = doubleValue(num, sz);
            goNext(trick, trickRating, 3, num, sz, played);
            return true;
        }
        return false;
    }
    bool createTriple(int num, int sz, bool played)
    {
        if((num==13 && sz>1) || num>=52 || (num%13)+sz>=15) //Aces and 2s aren't consecutive, so num==0 && sz>1 is bad
            return false;
        bool hasAll = true;
        vector<int> trick;
        for(int i=num; i<num+sz; i++)
        {
            int v = i;
            if(v%13 == 0) //we're checking an ace
                v -= 13;
            int has = 0;
            for(int j=0; j<4 && has<3; j++)
            {
                for(int k=0; k<cnt[j*13 + v] && has<3; k++)
                {
                    has++;
                    trick.push_back(j*13 + v);
                }
            }
            if(has < 3) //we're missing part of our (possibly consecutive) triple
            {
                hasAll = false;
                break;
            }
        }
        if(hasAll)
        {
            //check the triple first
            if(lastTrick.type==trick_info::ttype::invalid || lastTrick.type==trick_info::ttype::trip)
            {
                double trickRating = tripleValue(num, sz);
                goNext(trick, trickRating, 4, num, sz, played);
            }
            //check if a full house can be made
            int need = sz;
            for(int i=0; need>0 && i<13; i++)
            {
                int v = (i+2)%13; //use the lowest doubles possible (start at 3)
                int taken = 0;
                int has[4];
                for(int j=0; j<4; j++)
                {
                    has[j] = cnt[j*13+v] - count(trick.begin(), trick.end(), j*13+v);
                    taken += has[j];
                }
                if(taken >= 2)
                {
                    taken = 0;
                    for(int j=0; j<4; j++)
                    {
                        for(int k=0; taken<2 && k<has[j]; k++)
                        {
                            taken++;
                            trick.push_back(j*13+v);
                        }
                    }
                    need--;
                }
            }
            if(need==0 && (lastTrick.type==trick_info::ttype::invalid || lastTrick.type==trick_info::ttype::fullhouse)) //there are enough doubles to make a full house
            {
                double trickRating = fullHouseValue(num, sz);
                goNext(trick, trickRating, 4, num, sz, played);
            }
            return true;
        }
        return false;
    }
    //here are some bounds that can be adjusted if needed to speed up the search by a bit
    constexpr static int MAX_DOUB_LENGTH = 9, MAX_TRIP_LENGTH = 6, MAX_FLUSH_LENGTH = 13, MAX_STRAIGHT_LENGTH = 13, MAX_BOMB_LENGTH = 8;
    void createTricks(int pos1=0, int pos2=0, int pos3=5) //pos1 = trick type, pos2 = card value, pos3 = trick size
    {//these variables prevent finding the same trick multiple times, making the dfs a lot faster
        if(pos1 <= 0) //make straight flushes (note that it's possible to play flushes on any trick)
        {//note that once we make all the flushes we want, suits don't matter any more for any other tricks
            int i = 0;
            if(pos1 == 0)
                i = pos2;
            for(; i<52; i++)
            {
                int j = 5;
                if(pos1==0 && i==pos2)
                    j = pos3;
                for(;j<=MAX_FLUSH_LENGTH; j++)
                {
                    if(!createStraightFlush(i, j, true))
                        break; //if we can't make a 5 long starting here, then we definitely can't make a 6 long
                }
            }
        }
        if(pos1 <= 1) //make bombs (note that it's possible to play bombs on any trick)
        {
            int i = 0;
            if(pos1 == 1)
                i = pos2;
            for(; i<13; i++)
            {
                int j = 4;
                if(pos1==1 && i==pos2)
                    j = pos3;
                for(;j<=MAX_BOMB_LENGTH; j++)
                {
                    if(!createBomb(i, j, true))
                        break;
                }
            }
        }
        if(pos1 <= 2) //make straights
        {
            int i = 1;
            if(pos1 == 2)
                i = pos2;
            for(; i<=13; i++) //1-13 is used instead of 0-12 because Aces=0 so technically 12-0 is consecutive which might cause array out of bounds?
            {
                int j = 5;
                if(pos1==2 && i==pos2)
                    j = pos3;
                for(;j<=MAX_STRAIGHT_LENGTH; j++)
                {
                    if(!createStraight(i, j, true))
                        break;
                }
            }
        }
        if(pos1 <= 3) //make doubles
        {
            int i = 1;
            if(pos1 == 3)
                i = pos2;
            for(; i<=13; i++)
            {
                int j = 1;
                if(pos1==3 && i==pos2)
                    j = pos3;
                for(;j<=MAX_DOUB_LENGTH; j++)
                {
                    if(!createDouble(i, j, true))
                        break;
                }
            }
        }
        if(pos1 <= 4) //make triples and full houses
        {
            int i = 1;
            if(pos1 == 4)
                i = pos2;
            for(; i<=13; i++)
            {
                int j = 1;
                if(pos1==4 && i==pos2)
                    j = pos3;
                for(;j<=MAX_TRIP_LENGTH; j++)
                {
                    if(!createTriple(i, j, true))
                        break;
                }
            }
        }
        //the rest of the cards are singles
        for(int i=0; i<54; i++)
        {
            if(cnt[i])
                prv_rating += cnt[i] * singleValue(i);;
        }
        if(trickPlayed.size()) //the bot played something in the past
        {
            if(prv_rating + winFactor > best_rating[playerNum])
            {
                best_rating[playerNum] = prv_rating + winFactor;
                bestTrick[playerNum].cards = trickPlayed;
            }
        }
        else
        {
            if(prv_rating > best_rating[playerNum]) //the bot didn't play anything in the past
            {
                best_rating[playerNum] = prv_rating;
                bestTrick[playerNum].cards.clear(); //trickPlayed is empty
            }
        }
    }
    void playTrickFirst()
    {
        if(true) //make straight flushes (note that it's possible to play flushes on any trick)
        {//note that once we make all the flushes we want, suits don't matter any more for any other tricks
            for(int i=0; i<52; i++)
            {
                for(int j=5; j<=MAX_FLUSH_LENGTH; j++)
                {
                    if(lastTrick.isBomb && getStraightFlushPower(i, j)<=lastTrick.power)
                        continue;
                    if(!createStraightFlush(i, j, false))
                        break;
                }
            }
        }
        if(true) //make bombs (note that it's possible to play bombs on any trick)
        {
            for(int i=0; i<13; i++)
            {
                for(int j=4; j<=MAX_BOMB_LENGTH; j++)
                {
                    if(lastTrick.isBomb && getBombPower(i, j)<=lastTrick.power)
                        continue;
                    if(!createBomb(i, j, false))
                        break;
                }
            }
        }
        if(lastTrick.type==trick_info::ttype::invalid || lastTrick.type==trick_info::ttype::straight) //make straights
        {
            for(int i=1; i<=13; i++)
            {
                for(int j=5; j<=MAX_STRAIGHT_LENGTH; j++)
                {
                    if(lastTrick.type==trick_info::ttype::straight && (lastTrick.length!=j || (i+12)%13<=lastTrick.power))
                        continue;
                    if(!createStraight(i, j, false))
                        break;
                }
            }
        }
        if(lastTrick.type==trick_info::ttype::invalid || lastTrick.type==trick_info::ttype::doub) //make doubles
        {
            for(int i=1; i<=13; i++)
            {
                for(int j=1; j<=MAX_DOUB_LENGTH; j++)
                {
                    if(lastTrick.type==trick_info::ttype::doub && (lastTrick.length!=j || value(i+j-1)<=lastTrick.power))
                        continue;
                    if(!createDouble(i, j, false))
                        break;
                }
            }
        }
        if(lastTrick.type==trick_info::ttype::invalid || lastTrick.type==trick_info::ttype::trip ||
           lastTrick.type==trick_info::ttype::fullhouse) //make triples and full houses
        {
            for(int i=1; i<=13; i++)
            {
                for(int j=1; j<=MAX_TRIP_LENGTH; j++)
                {
                    if((lastTrick.type==trick_info::ttype::trip || lastTrick.type==trick_info::ttype::fullhouse)
                       && (lastTrick.length!=j || value(i+j-1)<=lastTrick.power))
                       continue;
                    if(!createTriple(i, j, false))
                        break;
                }
            }
        }
        //the rest of the cards are singles
        if(lastTrick.type==trick_info::ttype::invalid || lastTrick.type==trick_info::ttype::sing) //the AI will play a single now
        {
            vector<int> trick;
            for(int i=0; i<13; i++) //non joker singles
            {
                trick.clear();
                for(int j=0; j<4; j++)
                {
                    if(cnt[j*13 + i])
                    {
                        trick.push_back(j*13 + i);
                        break;
                    }
                }
                if(trick.size())
                {
                    if(lastTrick.type==trick_info::ttype::sing && lastTrick.power>=value(i))
                        continue;
                    double trickRating = singleValue(i);
                    goNext(trick, AI_SingleAffinity + trickRating, 0, 0, 0, false);
                }
            }
            //jokers
            if(cnt[52] && (lastTrick.type==trick_info::ttype::invalid || lastTrick.power<value(52)))
            {
                trick.clear();
                trick.push_back(52);
                double trickRating = singleValue(52);
                goNext(trick, AI_SingleAffinity + trickRating, 0, 0, 0, false);
            }
            if(cnt[53] && (lastTrick.type==trick_info::ttype::invalid || lastTrick.power<value(53)))
            {
                trick.clear();
                trick.push_back(53);
                double trickRating = singleValue(53);
                goNext(trick, AI_SingleAffinity + trickRating, 0, 0, 0, false);
            }
        }
    }
    void getRating() //Easter Egg: evaluates how good the computer thinks this hand is
    {
        prv_rating = 0;
        best_rating[playerNum] = MIN_RATING;
        trickPlayed.clear();
        createTricks();
        evalThreadEnded = true;
    }
    void getHint() //Easter Egg: evaluates what the computer thinks is the optimal move
    {
        prv_rating = 0;
        best_rating[playerNum] = MIN_RATING;
        playTrickFirst();
        if(turn != possession) //you can't pass if you go first
        {
            trickPlayed.clear();
            createTricks(); //check what's the best case scenario of playing nothing (also known as passing)
        }
        hintThreadEnded = true;
    }
    void getMove() //only the AI thread is allowed to call this
    {
        int startTurn = getTicks();
        //reset stuff
        prv_rating = 0;
        best_rating[playerNum] = MIN_RATING; //this is a global rating of all permutations of tricks tried so far
        //
        playTrickFirst(); //check what's the best case scenario of playing a trick
        if(turn != possession) //you can't pass if you go first
        {
            trickPlayed.clear();
            createTricks(); //check what's the best case scenario of playing nothing (also known as passing)
        }
        //note that only bestTrick.cards is filled out by the createTricks function to save time
        //the rest of the info in bestTrick has to be filled out manually with an external function
        //for some reason if we fill out bestTrick here then the operations go out of order
        int curTime = getTicks();
        if(curTime - startTurn < AI_MinTurnLength) //the game will be confusing if the AI finishes too quickly
            this_thread::sleep_for(chrono::milliseconds(AI_MinTurnLength - (curTime - startTurn)));
        for(auto &i: bestTrick[playerNum].cards) //do this here or else the AI card count will decrease before it plays the cards
            cnt[i]--;
        cardsInHand -= bestTrick[playerNum].cards.size();
        aiThreadEnded = true;
    }
};
