thread aiThread, evalThread, hintThread;
atomic<bool> aiThreadStarted, aiThreadEnded, evalThreadStarted, evalThreadEnded, hintThreadStarted, hintThreadEnded;
int NUM_PLAYERS = 4;
const int MAX_PLAYERS = 4;
int WIN_POINTS, PLAYER_HAND_OFFSET, MENU_OFFSET, BASE_FONT_SIZE;
int SCROLL_BAR_SIZE, INFO_BOX_OFFSET;
int SCROLL_SENSITIVITY;
bool SHOW_CHAT_TIME_STAMPS = 1;
int PLAYER_NUM = 0; //There are 4 players from 0-3, and 3 are bots. 0 is the player by default (don't change this because it might cause issues)
int pointsInPile, turn, possession, turnsWithoutPlay;
int AI_MinTurnLength = 2000, PointsToWin = 1000;
double AI_PointAffinity = 1; //multiplicative
double AI_Randomness = 0, AI_BombAffinity = 0, AI_SingleAffinity = 0, AI_DoubleAffinity = 0, AI_TripleAffinity = 0;
double AI_StraightAffinity = 0, AI_FullhouseAffinity = 0, AI_StraightFlushAffinity = 0; //additive
bool easterEggsEnabled = true;
struct MusicInfo
{
    Mix_Music *mus;
    int fadeIn, fadeOut; //so far fade out doesn't really do anything
    MusicInfo(const char *s, int a, int b)
    {
        mus = NULL;
        mus = Mix_LoadMUS(s);
        if(mus == NULL)
            println((string)"Mix_GetError(): " + Mix_GetError());
        fadeIn = a; //a=0 or b=0 effectively means don't fade in/out
        fadeOut = b;
    }
};
vector<MusicInfo> music;
atomic<int> curMusicPlaying(-1);
bool isMusicPlaying()
{
    if(curMusicPlaying!=-1 && music[curMusicPlaying].mus==NULL) //we don't want infinite error messages about mus equals NULL
        return true;
    return Mix_PlayingMusic();
}
void startMusic(int x)
{
    curMusicPlaying = x;
    if(x >= music.size())
    {
        println("Tried to play music track " + to_str(x) + ", but that is past the end of the MusicInfo vector");
    }
    else if(music[x].mus != NULL)
    {
        println("Now playing music track " + to_str(x));
        curMusicPlaying = x;
        Mix_FadeInMusic(music[x].mus, 1, music[x].fadeIn);
    }
    else
    {
        println("Tried to play music track " + to_str(x) + ", but mus equals NULL");
    }
}
void joinThread(thread &x)
{
    if(x.joinable())
        x.join();
}
void exitGame()
{
    println("Program received exit signal");
    SDL_Quit();
    sdl_settings::output_config();
    joinThread(aiThread);
    joinThread(evalThread);
    joinThread(hintThread);
    println("Program exited normally");
    exit(EXIT_SUCCESS);
}
void recolor(uint8_t *r, uint8_t *g, uint8_t *b)
{
    if(*r>=150 && *g>=150 && *b<=150)
    {
        *r = 255;
        *g = 255;
        *b = 0;
    }
}
void initGame()
{
    using namespace settings;
    music.push_back({"music/0.mp3", 5000, 0});
    music.push_back({"music/1.mp3", 0, 0});
    for(int i=0; i<=CARD_TYPES; i++)
    {
        t_cards[i] = loadTexture(("cards/" + to_str(i) + ".png").c_str());
        if(t_cards[i] == NULL)
            println("IMG_GetError(): " + (string)IMG_GetError());
    }
    SDL_SetWindowIcon(window, TTF_RenderText_Solid(font[5], "ZSY", {0, 255, 0}));
    int CW, CH;
    SDL_QueryTexture(t_cards[0], NULL, NULL, &CW, &CH);
    SPRITE_SCALE = BASE_CARD_H_RATIO / (CH / (double)WINDOW_H);
    CARD_W = CW * SPRITE_SCALE;
    CARD_H = CH * SPRITE_SCALE;
    println("Sprite scaling ratio: " + to_str(round(SPRITE_SCALE, 2)));
    MENU_OFFSET = WINDOW_W * 3/4;
    PLAYER_HAND_OFFSET = WINDOW_W * 1/10; //player's hand's offset from the left boundary of the window
    CARD_SPACE = CARD_W * 1/6; //space between consecutive cards
    CARD_STICK_OUT = CARD_H * 1/5; //how much does a card stick out if it's selected
    BASE_FONT_SIZE = WINDOW_H * 1/20;
    SCROLL_BAR_SIZE = WINDOW_W * 1/20;
    INFO_BOX_OFFSET = WINDOW_H * 3/4;
    SCROLL_SENSITIVITY = 30;
    WIN_POINTS = 1000;
}
