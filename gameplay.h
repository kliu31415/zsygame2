vector<hand> hands;
bool cardSelected[HAND_SIZE];
int points[4], declare[4]; //for this round only
int totalPoints[2]; //cumulative over every round
bool isOut[4];
enum class _gameStage{notDealt, dealt, inProgress} gameStage;
int outFirst, pointsStillLeft;
vector<pair<int, string> > chatLog;
double chatScroll, chatY;
int lookingAtHand; //equals PLAYER_NUM unless you cheat and look at other people's cards
void addToChatLog(string s)
{
    using namespace settings;
    chatLog.push_back(make_pair(getTicks()/1000, s));
    string a;
    if(SHOW_CHAT_TIME_STAMPS)
        a += "[" + seconds_to_str(chatLog.back().first) + "]";
    a += s;
    if(chatY > INFO_BOX_OFFSET - BASE_FONT_SIZE)
        chatScroll -= BASE_FONT_SIZE * multilineTextLength(a, WINDOW_W-MENU_OFFSET, BASE_FONT_SIZE*3/4) * 3/4;
}
void displayChatLog()
{
    using namespace settings;
    int CHAT_FONT_SIZE = BASE_FONT_SIZE * 3/4;
    chatScroll = min(chatScroll, 0.0);
    chatY = chatScroll;
    for(int i=0; i<chatLog.size(); i++)
    {
        string s;
        if(SHOW_CHAT_TIME_STAMPS)
            s += "[" + seconds_to_str(chatLog[i].first) + "]";
        s += chatLog[i].second;
        drawLine(MENU_OFFSET, chatY, WINDOW_W, chatY, 0, 0, 0);
        chatY += CHAT_FONT_SIZE * drawMultilineText(s, MENU_OFFSET, chatY, WINDOW_W - MENU_OFFSET, CHAT_FONT_SIZE);
    }
}
void setWinFactor()
{
    double &winFactor = hands[turn].winFactor;
    winFactor = 0;
    int phsz = hands[possession].getNumCards(); //number of cards the person who has possession has
    if(turn == possession){} //we're the first to play so winFactor doesn't matter anyway
    else if((possession + turn) & 1) //the other team has possession
    {
        winFactor = 10;
        if(turnsWithoutPlay == 0)
        {
            if(!isOut[(turn+2)%4]) //our partner has yet to play on this
                winFactor -= 10;
            if(isOut[possession]) //the player is out anyway
                winFactor = 0;
        }
        if(!isOut[possession]) //the next few lines of code don't matter if the person with possession is already out
        {
            winFactor += declare[possession] / sqrt(phsz+10); //it's bad if the opponent who declares gets out first
            winFactor += 15 - phsz/2; //try to stop people who are about to get out
            winFactor += max(0, 10 - phsz); //more urgent when the player has less cards
        }
        winFactor += pointsInPile/2 * AI_PointAffinity;
    }
    else
    {
        winFactor = -10; //our team has possession
        if(lastTrick.isBomb) //don't bomb allies!
            winFactor -= 30;
        else winFactor -= 2*lastTrick.power; //it doesn't matter that much if we play on a teammate's non-bomb low tricks
        if(phsz != 0)
            winFactor -= declare[possession] / sqrt(phsz+10); //it's good if the ally who declares gets out first
    }
    assert(!isnan(winFactor)); //divide by 0 is bad
    println("Win factor: " + to_str(winFactor));
}
void operateAIthread() //this needs(?) to be a function outside of a struct in order for a thread to use it
{
    setWinFactor();
    hands[turn].getMove();
}
void initNewRound();
bool initNewTrick(bool first = false) //returns whether a new round is started or not
{
    if(accumulate(isOut, isOut + NUM_PLAYERS, 0u) == 3) //this round is over
    {
        int team;
        if(isOut[0] && isOut[2])
            team = 0;
        else team = 1;
        if((outFirst%2) != team) //the player who got out first is on the same team as the player who got out last
        {
            if(possession == (outFirst+2)%4)
                totalPoints[(team+1)%2] += pointsInPile;
        }
        else totalPoints[team] += pointsInPile;
        totalPoints[team] += pointsStillLeft - pointsInPile;
        addToChatLog("The round has finished");
        initNewRound();
        return true;
    }
    else
    {
        if(!first)
        {
            addToChatLog("P" + to_str(possession) + " wins the trick, which has " + to_str(pointsInPile) + " points. A new trick starts");
        }
        println("New trick started");
        if(outFirst!=-1 && (outFirst%2==possession%2 || isOut[possession])) //these points are secured
        {
            totalPoints[possession%2] += pointsInPile;
        }
        else points[possession] += pointsInPile;
        pointsStillLeft -= pointsInPile;
        pointsInPile = 0;
        lastTrick = trick_info();
        assert(!isOut[turn]); //this should've been handled at the beginning of the operateGameInProgress function
        possession = turn;
        turnsWithoutPlay = 0;
        return false;
    }
}
void findWhichPlayerGoesFirst()
{
    if(outFirst != -1) //if outFirst==-1, it means this is the first round in the game so 3 of spades decides
    {
        turn = possession = outFirst;
        addToChatLog("P" + to_str(outFirst) + " got out first last round and goes first");
        return;
    }
    vector<int> goesFirst;
    for(int i=0; i<4; i++)
        goesFirst.push_back(i);
    int order[]{0, 1, 2, 3}; //spades, hearts, diamonds, clubs (the order array will depend on how the card sprites are arranged)
    for(int i=0; i<13 && goesFirst.size()>1; i++)
    {
        int v = (i+2) % 13;
        for(int j=0; j<4 && goesFirst.size()>1; j++)
        {
            bool oneHas = false; //if no one in contention has this, then skip to the next one
            for(auto &k: goesFirst)
            {
                if(hands[k].cnt[order[j]*13 + v] > 0)
                    oneHas = true;
                //cout << hands[k].cnt[order[j]*13 + v] << "\n";
            }
            if(!oneHas) //only 2 players can be in contention after 3 of spades
            {
                addToChatLog("Neither P" + to_str(goesFirst[0]) + " nor P" + to_str(goesFirst[1]) + " have the " + getCardName(order[j]*13 + v));
                continue;
            }
            for(int k=0; k<goesFirst.size(); k++)
            {
                if(hands[goesFirst[k]].cnt[order[j]*13 + v] == 0)
                {
                    goesFirst.erase(goesFirst.begin() + k);
                    k--;
                }
            }
            if(goesFirst.size() == 1)
            {
                addToChatLog("P" + to_str(goesFirst[0]) + " has the " + getCardName(order[j]*13 + v) + " and goes first");
                possession = turn = goesFirst[0];
            }
            else addToChatLog("Both P" + to_str(goesFirst[0]) + " and P" + to_str(goesFirst[1]) + " have the " + getCardName(order[j]*13 + v));
        }
    }
}
void dealCards()
{
    hands.clear();
    for(int i=0; i<NUM_PLAYERS; i++)
        hands.push_back({i});
    int deck[HAND_SIZE*NUM_PLAYERS];
    for(int i=0; i<HAND_SIZE*NUM_PLAYERS; i++)
    {
        deck[i] = i/2;
    }
    random_shuffle(deck, deck+HAND_SIZE*NUM_PLAYERS, randuzm);
    for(int i=0; i<NUM_PLAYERS; i++)
    {
        for(int j=0; j<HAND_SIZE; j++)
        {
            hands[i].cnt[deck[i*HAND_SIZE+j]]++;
        }
    }
    for(int i=0; i<HAND_SIZE; i++) //the player's cards are primarily stored in a vector instead of a cnt array so they can be rearranged by the player
        hands[PLAYER_NUM].cards.push_back(deck[PLAYER_NUM*HAND_SIZE + i]);
}
void initNewRound()
{
    gameStage = _gameStage::notDealt;
    fill(points, points + NUM_PLAYERS, 0);
    fill(declare, declare + NUM_PLAYERS, 0);
    fill(cardSelected, cardSelected + HAND_SIZE, 0);
    fill(isOut, isOut + NUM_PLAYERS, false);
    pointsInPile = 0;
    pointsStillLeft = 200;
    outFirst = -1;
    lookingAtHand = PLAYER_NUM;
    dealCards();
    aiThreadStarted = aiThreadEnded = evalThreadStarted = evalThreadEnded = hintThreadStarted = hintThreadEnded = false;
    println("New round started");
}
void initNewGame()
{
    currentState = gameState::inGame;
    fill(totalPoints, totalPoints+2, 0);
    chatLog.clear();
    chatScroll = 0;
    addToChatLog("New game started");
    initNewRound();
}
void showPlayerInfo(int pnum, int x, int y)
{
    using namespace settings;
    if(pnum > 0) //only show card counts for the bots
    {
        drawCard(54, x, y);
        int c = hands[pnum].getNumCards();
        if(c > 10)
            drawText("C:MANY", x, y+BASE_FONT_SIZE*2, BASE_FONT_SIZE);
        else drawText("C:" + to_str(c), x, y+BASE_FONT_SIZE*2, BASE_FONT_SIZE);
    }
    drawText("P" + to_str(pnum), x, y, BASE_FONT_SIZE);
    drawText("PTS:" + to_str(points[pnum]), x, y+BASE_FONT_SIZE, BASE_FONT_SIZE);
}
void groupCardsLeft()
{
    int pos = -1;
    for(int i=0; i<hands[PLAYER_NUM].cards.size(); i++)
    {
        if(cardSelected[i])
        {
            if(pos == -1)
                pos = i;
            rotate(hands[PLAYER_NUM].cards.begin() + pos, hands[PLAYER_NUM].cards.begin() + i, hands[PLAYER_NUM].cards.begin() + i + 1);
            cardSelected[i] = false;
            cardSelected[pos] = true;
            pos++;
        }
    }
}
void shiftCardsLeft()
{
    bool ungrouped = false;
    for(int i=0; i<hands[PLAYER_NUM].cards.size(); i++)
    {
        if(cardSelected[i])
        {
            if(ungrouped)
            {
                swap(hands[PLAYER_NUM].cards[i-1], hands[PLAYER_NUM].cards[i]);
                cardSelected[i] = false;
                cardSelected[i-1] = true;
            }
        }
        else ungrouped = true;
    }
}
void shiftCardsRight()
{
    bool ungrouped = false;
    for(int i=hands[PLAYER_NUM].cards.size()-1; i>=0; i--)
    {
        if(cardSelected[i])
        {
            if(ungrouped)
            {
                swap(hands[PLAYER_NUM].cards[i+1], hands[PLAYER_NUM].cards[i]);
                cardSelected[i] = false;
                cardSelected[i+1] = true;
            }
        }
        else ungrouped = true;
    }
}
void playerGetsOut(int pnum) //can be either a player or a bot
{
    println("P" + to_str(pnum) + " got out");
    isOut[pnum] = true;
    int numOut = accumulate(isOut, isOut+NUM_PLAYERS, 0u);
    //note that pointsInPile aren't necessarily won when a player gets out unless that player's team swept
    if(numOut == 1)
    {
        outFirst = pnum;
        for(int i=0; i<4; i++)
        {
            if(declare[i] != 0)
            {
                if(i == pnum)
                {
                    totalPoints[pnum%2] += declare[pnum];
                    addToChatLog("P" + to_str(pnum) + " declared and got out first, gaining " + to_str(declare[pnum]) + " points");
                }
                else
                {
                    totalPoints[i%2] -= declare[i];
                    addToChatLog("P" + to_str(i) + " declared and didn't get out first, losing " + to_str(declare[i]) + " points");
                }
            }
        }
        int pts = points[pnum] + points[(pnum+2)%4];
        totalPoints[pnum%2] += pts;
        addToChatLog("P" + to_str(pnum) + " got out first, securing " + to_str(pts) + " points");
        points[pnum] = points[(pnum+2)%4] = 0;
    }
    else if(numOut <= 2)
    {
        totalPoints[pnum%2] += points[pnum];
        addToChatLog("P" + to_str(pnum) + " got out, securing " + to_str(points[pnum]) + " points");
        points[pnum] = 0;
        if(isOut[(pnum+2)%4]) //the player's ally is out too
        {
            totalPoints[pnum%2] += points[pnum] + pointsStillLeft - pointsInPile;
            for(int i=0; i<4; i++)
            {
                if(!isOut[i]) //2 people can both not be out, so only add pointsStillLeft + pointsInPile once
                {
                    addToChatLog("P" + to_str(i) + " didn't get out");
                    totalPoints[pnum%2] += points[i];
                    break;
                }
            }
        }
    }
    else //everyone else is already out
    {
        //I don't think anything needs to be done
    }
}
void playSelectedCards()
{
    vector<int> curTrick;
    for(int i=0; i<hands[PLAYER_NUM].cards.size(); i++)
    {
        if(cardSelected[i])
            curTrick.push_back(hands[PLAYER_NUM].cards[i]);
    }
    sort(curTrick.begin(), curTrick.end());
    trick_info x = getTrickInfo(curTrick);
    if(x.type==trick_info::ttype::invalid || (turnsWithoutPlay==0 && turn==possession && x.type==trick_info::ttype::none))
    {
        println("Invalid trick selected");
        return;
    }
    if(x.type!=trick_info::ttype::none && lastTrick.type!=trick_info::ttype::invalid)
    {
        if(x.type == lastTrick.type)
        {
            if(x.isBomb)
            {
                if(x.power <= lastTrick.power)
                {
                    println("Invalid trick selected");
                    return;
                }
            }
            else
            {
                if(x.length!=lastTrick.length || x.power<=lastTrick.power)
                {
                    println("Invalid trick selected");
                    return;
                }
            }
        }
        else
        {
            if(!x.isBomb)
            {
                println("Invalid trick selected");
                return;
            }
            else
            {
                if(lastTrick.isBomb)
                {
                    if(x.power <= lastTrick.power)
                    {
                        println("Invalid trick selected");
                        return;
                    }
                }
            }
        }
    }
    for(auto &i: curTrick) //we need this in case we use the getHint and evaluate easter eggs
        hands[PLAYER_NUM].cnt[i]--;
    for(int i=hands[PLAYER_NUM].cards.size()-1; i>=0; i--)
    {
        if(cardSelected[i])
        {
            hands[PLAYER_NUM].cards.erase(hands[PLAYER_NUM].cards.begin() + i);
            cardSelected[i] = false;
        }
    }
    if(x.type == trick_info::ttype::none)
    {
        addToChatLog("P" + to_str(PLAYER_NUM) + " passes");
        turnsWithoutPlay++;
    }
    else
    {
        turnsWithoutPlay = 0;
        possession = turn;
        lastTrick = x;
        lastTrick.cards = curTrick;
        sort(lastTrick.cards.begin(), lastTrick.cards.end(), cmpVal);
        addToChatLog("P" + to_str(PLAYER_NUM) + " plays " + lastTrick.name);
        pointsInPile += pointsInCards(lastTrick.cards);
        if(hands[PLAYER_NUM].getNumCards() == 0)
            playerGetsOut(PLAYER_NUM);
    }
    turn = (turn+1) % 4;
}
void operateScrollWheel()
{
    const uint8_t *keystate = SDL_GetKeyboardState(NULL);
    if(keystate[SDL_SCANCODE_C])
        chatScroll += input.wheel.y * SCROLL_SENSITIVITY;
    if(keystate[SDL_SCANCODE_V])
    {
        setVolume(Mix_VolumeMusic(-1) + 2*input.wheel.y);
    }
    if(keystate[SDL_SCANCODE_S])
    {
        CARD_W *= (1 + 0.02*input.wheel.y);
        CARD_H *= (1 + 0.02*input.wheel.y);
        CARD_SPACE = CARD_W * 1/6; //space between consecutive cards
        CARD_STICK_OUT = CARD_H * 1/5; //how much does a card stick out if it's selected
    }
    if(keystate[SDL_SCANCODE_F])
    {
        int newSz = BASE_FONT_SIZE*(1 + 0.02*input.wheel.y);
        if(newSz == BASE_FONT_SIZE) //a small integer *1.02 may equal itself because it truncates
            newSz++;
        if(newSz <= 0) //apparently drawing text crashes the program if the font size is 0
            newSz = 1;
        BASE_FONT_SIZE = newSz;
    }
}
void operateEvalThread(const uint8_t *keystate) //Easter Egg
{
    if(!easterEggsEnabled)
        return;
    static int lastEval = 0;
    if(evalThreadStarted && evalThreadEnded)
    {
        evalThread.join();
        println("The AI peeks at your hand and gives it a rating of " + to_str(best_rating[PLAYER_NUM]));
        evalThreadStarted = false;
        evalThreadEnded = false;
    }
    if(keystate[SDL_SCANCODE_F11])
    {
        if(getTicks()-lastEval>500 && !evalThreadStarted)
        {
            lastEval = getTicks();
            evalThreadStarted = true;
            evalThread = thread([]()->void{hands[PLAYER_NUM].getRating();});
        }
    }
}
void operateHintThread(const uint8_t *keystate) //Easter Egg
{
    if(!easterEggsEnabled)
        return;
    static int lastHint = 0;
    if(hintThreadStarted && hintThreadEnded)
    {
        hintThread.join();
        trick_info &t = bestTrick[PLAYER_NUM];
        vector<int> c = t.cards; //for some reason these 3 lines don't work well if called in the other thread
        t = getTrickInfo(c);
        t.cards = c;
        if(t.type == trick_info::ttype::none)
            println("The AI thinks you should pass");
        else println("The AI thinks you should play " + t.name);
        hintThreadStarted = false;
        hintThreadEnded = false;
    }
    if(keystate[SDL_SCANCODE_F10] && !isOut[PLAYER_NUM] && turn==PLAYER_NUM) //turn==PLAYER_NUM because winFactor is a shared variable
    {
        if(getTicks()-lastHint>500 && !hintThreadStarted)
        {
            lastHint = getTicks();
            hintThreadStarted = true;
            setWinFactor();
            hintThread = thread([]()->void{hands[PLAYER_NUM].getHint();});
        }
    }
}
void redealCards(const uint8_t *keystate) //Easter Egg
{
    if(!easterEggsEnabled)
        return;
    static int lastRedeal = 0;
    if(keystate[SDL_SCANCODE_F12])
    {
        if(getTicks() - lastRedeal > 500)
        {
            lastRedeal = getTicks();
            dealCards();
            if(randf() < 0.8)
                println("You cheat and redeal the cards. The AIs somehow don't notice.");
            else
            {
                if(randf() < 0.8)
                    println("The AI sitting next to you notices you redealing the cards and gives you an angry stare. However, it doesn't try to stop you.");
                else println("The AI who is your partner sees you redealing the deck but simply winks at you");
            }
        }
    }
}
void checkIfPlayerPeeks(const uint8_t *keystate) //Easter Egg
{
    if(!easterEggsEnabled)
        return;
    if(keystate[SDL_SCANCODE_ESCAPE])
        lookingAtHand = PLAYER_NUM;
    for(int i=1; i<NUM_PLAYERS; i++)
    {
        if(keystate[SDL_SCANCODE_F1 + i - 1] && lookingAtHand!=i)
        {
            lookingAtHand = i;
            if(randf() < 0.8)
                println("You look at P" + to_str(i) + "'s hand. The AI doesn't notice");
            else
            {
                if(randf() < 0.8)
                    println("P" + to_str(i) + " catches you looking at its hand. It appears irritated but does not stop you");
                else
                {
                    lookingAtHand = PLAYER_NUM;
                    println("P" + to_str(i) + " shoves you away and prevents you from looking at its hand. How rude!");
                }
            }
        }
    }
}
void displayHand() //Contains an easter egg
{
    using namespace settings;
    if(lookingAtHand == PLAYER_NUM) //Not an easter egg
    {
        for(int i=0; i<hands[PLAYER_NUM].cards.size(); i++)
        {
            if(cardSelected[i])
                drawCard(hands[PLAYER_NUM].cards[i], PLAYER_HAND_OFFSET + CARD_SPACE*i, WINDOW_H - CARD_H - CARD_STICK_OUT);
            else drawCard(hands[PLAYER_NUM].cards[i], PLAYER_HAND_OFFSET + CARD_SPACE*i, WINDOW_H - CARD_H);
        }
    }
    else //Easter Egg
    {
        vector<int> dispHand;
        int cur = 0;
        for(auto &i: hands[lookingAtHand].cnt)
        {
            for(int j=0; j<i; j++)
                dispHand.push_back(cur);
            cur++;
        }
        //sorting the vector every time isn't optimal, but it's a light operation so whatever
        stable_sort(dispHand.begin(), dispHand.end(), cmpVal);
        for(int i=0; i<dispHand.size(); i++)
        {
            drawCard(dispHand[i], PLAYER_HAND_OFFSET + CARD_SPACE*i, WINDOW_H - CARD_H);
        }
    }
}
void operateGameInProgress()
{
    using namespace settings;
    rect PLAY_BOX{MENU_OFFSET, INFO_BOX_OFFSET+BASE_FONT_SIZE, settings::WINDOW_W-MENU_OFFSET, BASE_FONT_SIZE}; //where the player clicks "PLAY/PASS/etc"
    rect OPT2_BOX{MENU_OFFSET, INFO_BOX_OFFSET+BASE_FONT_SIZE*2, settings::WINDOW_W-MENU_OFFSET, BASE_FONT_SIZE};
    while(isOut[turn])
    {
        turn = (turn+1) % 4;
        turnsWithoutPlay++;
    }
    if(turnsWithoutPlay >= NUM_PLAYERS-1) //whoever last had possession wins this trick wins it (>= in case the player got out this turn)
    {
        if(initNewTrick())
            return;
    }
    if(turn != PLAYER_NUM) //handle AIs
    {
        if(!aiThreadStarted)
        {
            aiThread = thread(operateAIthread);
            aiThreadStarted = true;
            println("AI thread for P" + to_str(turn) + " started");
            aiThreadEnded = false;
        }
        if(aiThreadEnded)
        {
            aiThread.join();
            trick_info &t = bestTrick[turn];
            vector<int> c = t.cards; //for some reason these 3 lines don't work well if called in the AI thread
            t = getTrickInfo(c);
            t.cards = c;
            println("AI thread for P" + to_str(turn) + " finished");
            if(t.type == trick_info::ttype::none)
            {
                addToChatLog("P" + to_str(turn) + " passes");
                turnsWithoutPlay++;
            }
            else
            {
                addToChatLog("P" + to_str(turn) + " plays " + t.name);
                lastTrick = t;
                turnsWithoutPlay = 0;
                possession = turn;
                pointsInPile += pointsInCards(lastTrick.cards);
                if(hands[turn].getNumCards() == 0)
                    playerGetsOut(turn);
            }
            turn = (turn+1) % 4;
            aiThreadStarted = false;
            aiThreadEnded = false;
        }
    }
    /*The input needs to be handled after the AI threads.
    If the AI wins possession right after the player clicks the "pass" button, initNewTrick() won't be called in between
    and lastTrick won't be reset, so the AI might pass even if it has possession if it can't play
    (Ex. lastTrick.isBomb==true and the AI has no more bombs*/
    while(SDL_PollEvent(&input))
    {
        switch(input.type)
        {
        case SDL_QUIT:
            exitGame();
            break;
        case SDL_MOUSEWHEEL:
            operateScrollWheel();
            break;
        case SDL_MOUSEBUTTONDOWN:
            if(input.button.button != SDL_BUTTON_LEFT)
                break;
            for(int i=hands[PLAYER_NUM].cards.size()-1; i>=0; i--)
            {
                if(cardSelected[i])
                {
                    if(mouseInRect(PLAYER_HAND_OFFSET + CARD_SPACE*i, WINDOW_H-CARD_H-CARD_STICK_OUT, CARD_W, CARD_H))
                    {
                        cardSelected[i] = false;
                        break;
                    }
                }
                else
                {
                    if(mouseInRect(PLAYER_HAND_OFFSET + CARD_SPACE*i, WINDOW_H-CARD_H, CARD_W, CARD_H))
                    {
                        cardSelected[i] = true;
                        break;
                    }
                }
            }
            if(turn==0 && mouseInRect(&PLAY_BOX))
            {
                playSelectedCards();
            }
            if(mouse_x>MENU_OFFSET && mouse_y<INFO_BOX_OFFSET)
            {
                chatScroll -= max(0.0, chatY - INFO_BOX_OFFSET);
            }
            break;
        case SDL_KEYDOWN:
            switch(input.key.keysym.sym)
            {
            case SDLK_SPACE:
                groupCardsLeft();
                break;
            case SDLK_LEFT:
                shiftCardsLeft();
                break;
            case SDLK_RIGHT:
                shiftCardsRight();
                break;
            case SDLK_DOWN:
                fill(cardSelected, cardSelected + hands[PLAYER_NUM].cards.size(), false);
                break;
            case SDLK_1:
                fill(cardSelected, cardSelected + hands[PLAYER_NUM].cards.size(), false);
                stable_sort(hands[0].cards.begin(), hands[0].cards.end(), cmpVal);
                break;
            case SDLK_2:
                fill(cardSelected, cardSelected + hands[PLAYER_NUM].cards.size(), false);
                stable_sort(hands[0].cards.begin(), hands[0].cards.end());
                break;
            }
            break;
        }
    }
    renderClear(50, 200, 50);
    displayHand();
    for(int i=0; i<lastTrick.cards.size(); i++)
    {
        drawCard(lastTrick.cards[i], CARD_W*2 + CARD_SPACE*i, WINDOW_H/2 - CARD_H/2);
    }
    showPlayerInfo(0, 0, WINDOW_H - BASE_FONT_SIZE*3);
    showPlayerInfo(1, 0, WINDOW_H/2 - CARD_H/2);
    showPlayerInfo(2, MENU_OFFSET/2 - CARD_W/2, 0);
    showPlayerInfo(3, MENU_OFFSET - CARD_W, WINDOW_H/2 - CARD_H/2);
    fillRect(MENU_OFFSET, 0, WINDOW_W-MENU_OFFSET, WINDOW_H, 200, 200, 200); //we don't want cards to be drawn over the menu if the cards get too big
    drawText("POINTS IN PILE:" + to_str(pointsInPile), CARD_W*2, WINDOW_H/2 - CARD_H/2 - BASE_FONT_SIZE, BASE_FONT_SIZE);
    displayChatLog();
    fillRect(MENU_OFFSET, INFO_BOX_OFFSET, WINDOW_W-MENU_OFFSET, WINDOW_H-INFO_BOX_OFFSET, 120, 220, 220);
    for(int i=0; i<2; i++)
        drawText("T" + to_str(i+1) + ":" + to_str(totalPoints[i]), MENU_OFFSET + BASE_FONT_SIZE*i*4, INFO_BOX_OFFSET, BASE_FONT_SIZE);
    fillRect(&PLAY_BOX, 255, 255, 0, 150);
    if(!isOut[0])
    {
        if(turn == 0)
        {
            if(accumulate(cardSelected, cardSelected + hands[PLAYER_NUM].cards.size(), 0) == 0)
                drawText("PASS", PLAY_BOX.x, PLAY_BOX.y, BASE_FONT_SIZE);
            else drawText("PLAY", PLAY_BOX.x, PLAY_BOX.y, BASE_FONT_SIZE);
        }
        else drawText("----", PLAY_BOX.x, PLAY_BOX.y, BASE_FONT_SIZE);
    }
    else drawText("----", PLAY_BOX.x, PLAY_BOX.y, BASE_FONT_SIZE);
    //Easter Eggs
    const uint8_t *keystate = SDL_GetKeyboardState(NULL);
    operateEvalThread(keystate);
    operateHintThread(keystate);
    checkIfPlayerPeeks(keystate);
}
void operateGameNotDealt()
{
    using namespace settings;
    rect PLAY_BOX{MENU_OFFSET, INFO_BOX_OFFSET+BASE_FONT_SIZE, settings::WINDOW_W-MENU_OFFSET, BASE_FONT_SIZE}; //where the player clicks "PLAY/PASS/etc"
    rect OPT2_BOX{MENU_OFFSET, INFO_BOX_OFFSET+BASE_FONT_SIZE*2, settings::WINDOW_W-MENU_OFFSET, BASE_FONT_SIZE};
    renderClear(50, 200, 50);
    fillRect(MENU_OFFSET, 0, WINDOW_W-MENU_OFFSET, WINDOW_H, 200, 200, 200);
    displayChatLog();
    fillRect(MENU_OFFSET, INFO_BOX_OFFSET, WINDOW_W-MENU_OFFSET, WINDOW_H-INFO_BOX_OFFSET, 120, 220, 220);
    for(int i=0; i<2; i++)
        drawText("T" + to_str(i+1) + ":" + to_str(totalPoints[i]), MENU_OFFSET + BASE_FONT_SIZE*i*4, INFO_BOX_OFFSET, BASE_FONT_SIZE);
    fillRect(&PLAY_BOX, 255, 255, 0, 150);
    fillRect(&OPT2_BOX, 255, 0, 255, 150);
    drawText("DEAL CARDS", PLAY_BOX.x, PLAY_BOX.y, BASE_FONT_SIZE);
    if(declare[PLAYER_NUM] == 0) //hasn't already declared
        drawText("BLIND DECLARE", OPT2_BOX.x, OPT2_BOX.y, BASE_FONT_SIZE);
    else drawText("-------------", OPT2_BOX.x, OPT2_BOX.y, BASE_FONT_SIZE); //has already declared
    while(SDL_PollEvent(&input))
    {
        switch(input.type)
        {
        case SDL_QUIT:
            exitGame();
            break;
        case SDL_MOUSEWHEEL:
            operateScrollWheel();
            break;
        case SDL_MOUSEBUTTONDOWN:
            if(input.button.button != SDL_BUTTON_LEFT)
                break;
            if(mouseInRect(&PLAY_BOX))
            {
                addToChatLog("Cards have been dealt");
                gameStage = _gameStage::dealt;
                return;
            }
            if(mouseInRect(&OPT2_BOX))
            {
                if(declare[PLAYER_NUM] == 0)
                {
                    addToChatLog("P" + to_str(PLAYER_NUM) + " blind declares");
                    declare[PLAYER_NUM] = 200;
                }
            }
            if(mouse_x>MENU_OFFSET && mouse_y<INFO_BOX_OFFSET)
            {
                chatScroll -= max(0.0, chatY - INFO_BOX_OFFSET);
            }
            break;
        }
    }
}
void operateGameDealt()
{
    using namespace settings;
    rect PLAY_BOX{MENU_OFFSET, INFO_BOX_OFFSET+BASE_FONT_SIZE, settings::WINDOW_W-MENU_OFFSET, BASE_FONT_SIZE}; //where the player clicks "PLAY/PASS/etc"
    rect OPT2_BOX{MENU_OFFSET, INFO_BOX_OFFSET+BASE_FONT_SIZE*2, settings::WINDOW_W-MENU_OFFSET, BASE_FONT_SIZE};
    renderClear(50, 200, 50);
    displayHand();
    for(int i=0; i<lastTrick.cards.size(); i++)
    {
        drawCard(lastTrick.cards[i], CARD_W*2 + CARD_SPACE*i, WINDOW_H/2 - CARD_H/2);
    }
    showPlayerInfo(0, 0, WINDOW_H - BASE_FONT_SIZE*3);
    showPlayerInfo(1, 0, WINDOW_H/2 - CARD_H/2);
    showPlayerInfo(2, MENU_OFFSET/2 - CARD_W/2, 0);
    showPlayerInfo(3, MENU_OFFSET - CARD_W, WINDOW_H/2 - CARD_H/2);
    fillRect(MENU_OFFSET, 0, WINDOW_W-MENU_OFFSET, WINDOW_H, 200, 200, 200); //we don't want cards to be drawn over the menu if the cards get too big
    displayChatLog();
    fillRect(MENU_OFFSET, INFO_BOX_OFFSET, WINDOW_W-MENU_OFFSET, WINDOW_H-INFO_BOX_OFFSET, 120, 220, 220);
    for(int i=0; i<2; i++)
        drawText("T" + to_str(i+1) + ":" + to_str(totalPoints[i]), MENU_OFFSET + BASE_FONT_SIZE*i*4, INFO_BOX_OFFSET, BASE_FONT_SIZE);
    fillRect(&PLAY_BOX, 255, 255, 0, 150);
    fillRect(&OPT2_BOX, 255, 0, 255, 150);
    drawText("START", PLAY_BOX.x, PLAY_BOX.y, BASE_FONT_SIZE);
    if(declare[PLAYER_NUM] == 0) //hasn't already declared
        drawText("DECLARE", OPT2_BOX.x, OPT2_BOX.y, BASE_FONT_SIZE);
    else drawText("-------", OPT2_BOX.x, OPT2_BOX.y, BASE_FONT_SIZE); //has already declared
    while(SDL_PollEvent(&input))
    {
        switch(input.type)
        {
        case SDL_QUIT:
            exitGame();
            break;
        case SDL_MOUSEWHEEL:
            operateScrollWheel();
            break;
        case SDL_MOUSEBUTTONDOWN:
            if(input.button.button != SDL_BUTTON_LEFT)
                break;
            for(int i=hands[PLAYER_NUM].cards.size()-1; i>=0; i--)
            {
                if(cardSelected[i])
                {
                    if(mouseInRect(PLAYER_HAND_OFFSET + CARD_SPACE*i, WINDOW_H-CARD_H-CARD_STICK_OUT, CARD_W, CARD_H))
                    {
                        cardSelected[i] = false;
                        break;
                    }
                }
                else
                {
                    if(mouseInRect(PLAYER_HAND_OFFSET + CARD_SPACE*i, WINDOW_H-CARD_H, CARD_W, CARD_H))
                    {
                        cardSelected[i] = true;
                        break;
                    }
                }
            }
            if(mouseInRect(&PLAY_BOX))
            {
                addToChatLog("First trick started");
                findWhichPlayerGoesFirst();
                initNewTrick(true);
                gameStage = _gameStage::inProgress;
                return;
            }
            if(mouseInRect(&OPT2_BOX))
            {
                if(declare[PLAYER_NUM] == 0)
                {
                    addToChatLog("P" + to_str(PLAYER_NUM) + " declares");
                    declare[PLAYER_NUM] = 100;
                }
            }
            break;
        case SDL_KEYDOWN:
            switch(input.key.keysym.sym)
            {
            case SDLK_SPACE:
                groupCardsLeft();
                break;
            case SDLK_LEFT:
                shiftCardsLeft();
                break;
            case SDLK_RIGHT:
                shiftCardsRight();
                break;
            case SDLK_DOWN:
                fill(cardSelected, cardSelected + hands[PLAYER_NUM].cards.size(), false);
                break;
            case SDLK_1:
                fill(cardSelected, cardSelected + hands[PLAYER_NUM].cards.size(), false);
                stable_sort(hands[0].cards.begin(), hands[0].cards.end(), cmpVal);
                break;
            case SDLK_2:
                fill(cardSelected, cardSelected + hands[PLAYER_NUM].cards.size(), false);
                stable_sort(hands[0].cards.begin(), hands[0].cards.end());
                break;
            }
            break;
        }
    }
    //EASTER EGGS!
    const uint8_t *keystate = SDL_GetKeyboardState(NULL);
    redealCards(keystate);
    operateEvalThread(keystate);
    checkIfPlayerPeeks(keystate);
}
void operateGame()
{
    using namespace settings;
    switch(gameStage)
    {
    case _gameStage::notDealt:
        operateGameNotDealt();
        break;
    case _gameStage::dealt:
        operateGameDealt();
        break;
    case _gameStage::inProgress:
        operateGameInProgress();
        break;
    }
    updateScreen();
}
