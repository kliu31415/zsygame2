const int HAND_SIZE = 27, CARD_TYPES = 54;
double CARD_W, CARD_H, CARD_SPACE;
int CARD_STICK_OUT;
texture *t_cards[CARD_TYPES+1]; //the +1 is for the image that represents the back of the card
void drawCard(int card, int x, int y)
{
    renderCopy(t_cards[card], x, y, CARD_W, CARD_H);
}
void showTrick(vector<pair<vector<int>, double> > &cur, int &X, int &Y, int space)
{
    for(auto &i: cur)
    {
        if(X + CARD_W + space*(i.first.size()-1) >= settings::WINDOW_W)
        {
            X = 0;
            Y += CARD_H*1.1;
        }
        for(auto &j: i.first)
        {
            drawCard(j, X, Y);
            X += space;
        }
        X += CARD_W;
    }
}
int pointsInCards(vector<int> &trick)
{
    int pts = 0;
    for(auto &i: trick)
    {
        switch(i % 13)
        {
        case 4: //5
            pts += 5;
            break;
        case 9: //10
        case 12: //King
            pts += 10;
            break;
        }
    }
    return pts;
}
string getRank(int x)
{
    static const char *rnk[]{"Ace", "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King"};
    return rnk[x%13];
}
string getSuit(int x)
{
    static const char *suit[]{"Spades", "Hearts", "Diamonds", "Clubs"};
    return suit[x/13];
}
string getCardName(int x)
{
    if(x < 52)
    {
        return getRank(x) + (string)" of " + getSuit(x);
    }
    if(x == 52)
        return "Little Joker";
    return "Big Joker";
}
int value(int x)
{
    if(x < 52) //not a joker
    {
        int v = (x+11)%13;
        if(v == 11) //Ace
            return 12;
        if(v == 12) //2
            return 15;
        return v;
    }
    if(x == 52) //little joker
        return 19;
    return 23; //big joker
}
struct trick_info
{
    string name;
    int power; //self-explanatory. For example, a 2 is more powerful than an Ace.
    enum class ttype{invalid, none, sing, doub, trip, fullhouse, straight, flush, bomb}type;
    int length; //usually denotes the length, e.g. JJQQKK would be 3 (consecutive doubles)
    bool isBomb;
    vector<int> cards; //may or may not be used, depending on whether the cards need to be seen or not
    trick_info()
    {
        type = ttype::invalid;
        isBomb = false;
        length = 0;
        power = 0;
        cards.clear();
        name = "";
    }
};
int getStraightFlushPower(int num, int sz)
{
    return sz*26 + 13 + (num+11)%13;
}
int getBombPower(int num, int sz)
{
    return sz*26 + (num+11)%13;
}
template<class T> trick_info isStraight(T cards)
{
    trick_info cur;
    if(cards.size() < 5)
        return cur;
    vector<int> ranks; //we don't care about the suits
    for(auto &i: cards)
    {
        if(i >= 52)
            return cur;
        ranks.push_back((i+12) % 13);
    }
    sort(ranks.begin(), ranks.end());
    for(int i=1; i<ranks.size(); i++)
    {
        if(ranks[i] != ranks[i-1]+1)
            return cur;
    }
    cur.name = "Straight, " + to_str(cards.size()) + " long, " + getRank(ranks.back()+1) + " high";
    cur.power = ranks[0];
    cur.isBomb = false;
    cur.length = cards.size();
    cur.type = trick_info::ttype::straight;
    return cur;
}
template<class T> trick_info isStraightFlush(T cards)
{
    trick_info cur = isStraight(cards);
    if(cur.type == trick_info::ttype::invalid)
        return cur;
    int suit = *cards.begin() / 13;
    int lowest = 1e9;
    for(auto &i: cards)
    {
        lowest = min(lowest, (i+12)%13);
        if(i/13 != suit)
            return cur;
    }
    cur.name = "Straight flush, " + to_str(cards.size()) + " long, " + getRank(*(cards.end()-1)) + " high";
    cur.power = getStraightFlushPower(lowest, cards.size());
    cur.isBomb = true;
    cur.type = trick_info::ttype::flush;
    return cur;
}
template<class T> trick_info isBomb(T cards)
{
    trick_info cur;
    if(cards.size() < 4)
        return cur;
    int rnk = (*cards.begin() + 11) % 13;
    for(auto &i: cards)
    {
        if((i+11)%13!=rnk || i>=52)
            return cur;
    }
    cur.name = getRank(*(cards.end()-1)) + " bomb, " + to_str(cards.size()) + " long";
    cur.power = getBombPower(*cards.begin(), cards.size());
    cur.isBomb = true;
    cur.type = trick_info::ttype::bomb;
    return cur;
}
template<class T> trick_info isDouble(T cards)
{
    trick_info cur;
    if(cards.size()%2 != 0)
        return cur;
    vector<int> ranks; //we don't care about the suits
    for(auto &i: cards)
    {
        if(i >= 52)
            return cur;
        ranks.push_back((i+12) % 13);
    }
    sort(ranks.begin(), ranks.end());
    for(int i=0; i<ranks.size(); i+=2)
    {
        if(ranks[i]!=ranks[i+1] || (i!=ranks.size()-2 && ranks[i]+1!=ranks[i+2]))
            return cur;
    }
    cur.isBomb = false;
    cur.length = cards.size() / 2;
    cur.type = trick_info::ttype::doub;
    if(cards.size() == 2)
    {
        cur.name = "Double " + getRank((ranks[0]+1)%13) + "s";
        cur.power = value(*cards.begin()); //2s can't be demoted if doubles aren't consecutive
        return cur;
    }
    cur.name = "Consecutive Doubles, " + to_str(cur.length) + " long, " + getRank((ranks.back()+1)%13) + " high";
    cur.power = ranks[0];
    return cur;
}
template<class T> trick_info isTriple(T cards)
{
    trick_info cur;
    if(cards.size()%3 != 0)
        return cur;
    vector<int> ranks; //we don't care about the suits
    for(auto &i: cards)
    {
        if(i >= 52)
            return cur;
        ranks.push_back((i+12) % 13);
    }
    sort(ranks.begin(), ranks.end());
    for(int i=0; i<ranks.size(); i+=3)
    {
        if(ranks[i]!=ranks[i+1] || ranks[i]!=ranks[i+2] || (i!=ranks.size()-3 && ranks[i]+1!=ranks[i+3]))
            return cur;
    }
    cur.isBomb = false;
    cur.length = cards.size() / 3;
    cur.type = trick_info::ttype::trip;
    if(cards.size() == 3)
    {
        cur.name = "Triple " + getRank((ranks[0]+1)%13) + "s";
        cur.power = value(*cards.begin()); //2s can't be demoted if triples aren't consecutive
        return cur;
    }
    cur.name = "Consecutive Triples, " + to_str(cur.length) + " long, " + getRank((ranks.back()+1)%13) + " high";
    cur.power = ranks[0];
    return cur;
}
template<class T> trick_info isFullhouse(T cards)
{
    trick_info cur;
    int cnt[CARD_TYPES]{};
    for(auto &i: cards)
    {
        if(i >= 52)
            return cur;
        cnt[i]++;
    }
    multiset<int> C2, C3;
    int highest3 = -1;
    for(int i=0; i<13; i++)
    {
        vector<int> thisRank;
        for(int j=0; j<4; j++)
        {
            for(int k=0; k<cnt[j*13+i]; k++)
                thisRank.push_back(j*13 + i);
        }
        if(thisRank.size() == 2)
        {
            for(auto &j: thisRank)
                C2.insert(j);
        }
        else if(thisRank.size() == 3)
        {
            for(auto &j: thisRank)
                C3.insert(j);
            highest3 = max(highest3, (i+12)%13);
        }
        else if(thisRank.size() != 0)
            return cur;
    }
    int cons = C2.size() / 2;
    if(cons*3 != C3.size()) //we need the same number of triples and doubles
        return cur;
    trick_info tripleInfo = isTriple(C3);
    if(tripleInfo.type != trick_info::ttype::trip)
        return cur;
    cur.isBomb = false;
    cur.length = cards.size() / 5;
    cur.type = trick_info::ttype::fullhouse;
    if(cur.length == 1)
    {
        cur.name = "Full house, " + getRank(*C3.begin()) + "s holding " + getRank(*C2.begin()) + "s";
        cur.power = value(*C3.begin()); //2s can't be demoted if triples aren't consecutive
        return cur;
    }
    cur.name = "Consecutive Full Houses, " + to_str(cur.length) + " long, " + getRank((highest3+1)%13) + " high";
    cur.power = (*C3.begin() + 12) % 13;
    return cur;
}
template<class T> trick_info isSingle(T cards)
{
    trick_info cur;
    if(cards.size() != 1)
        return cur;
    cur.power = value(*cards.begin());
    cur.name = "A Single ";
    if(*cards.begin() >= 52)
        cur.name += getCardName(*cards.begin());
    else cur.name += getRank(*cards.begin());
    cur.isBomb = false;
    cur.type = trick_info::ttype::sing;
    return cur;
}
template<class T> trick_info getTrickInfo(T cards) //T is probably a multiset or vector
{
    trick_info cur;
    if(cards.size() == 0)
    {
        cur.name = "(Pass) (Also, you shouldn't be seeing this!)";
        cur.type = trick_info::ttype::none;
        return cur;
    }
    cur = isStraightFlush(cards);
    if(cur.type != trick_info::ttype::invalid)
        return cur;
    cur = isBomb(cards);
    if(cur.type != trick_info::ttype::invalid)
        return cur;
    cur = isStraight(cards);
    if(cur.type != trick_info::ttype::invalid)
        return cur;
    cur = isDouble(cards);
    if(cur.type != trick_info::ttype::invalid)
        return cur;
    cur = isTriple(cards);
    if(cur.type != trick_info::ttype::invalid)
        return cur;
    cur = isFullhouse(cards);
    if(cur.type != trick_info::ttype::invalid)
        return cur;
    cur = isSingle(cards);
    if(cur.type != trick_info::ttype::invalid)
        return cur;
    return cur; //invalid trick
}
