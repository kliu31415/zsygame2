#define __GAME__VERSION__ "0.1.0"
#include <iostream>
#include <algorithm>
#include <set>
#include <atomic>
#include <cstring>
#include <cassert>
#include <csignal>
#include <cstdarg>
#include "sdl_base.h"
#include "sdl_base.cpp"
using namespace std;
enum class gameState{inGame, menu}currentState = gameState::inGame;
#include "card.h"
#include "gamebase.h"
#include "ai.h"
#include "gameplay.h"
#include "menu.h"
int main(int argc, char **argv)
{
    if(argc > 1) //why are you passing arguments to this program if it doesn't support any? (jk easter eggs exist)
    {
        for(int i=1; i<argc; i++)
        {
            if(!strcmp("-noeastereggs", argv[i]))
                easterEggsEnabled = false;
            else println("Unknown argument: " + to_str(argv[i]));
        }
    }
    sdl_settings::load_config();
    initSDL();
    showLoadingScreen();
    initGame();
    initMenu();
    while(true)
    {
        switch(currentState)
        {
        case gameState::menu:
            operateMenu();
            break;
        case gameState::inGame:
            operateGame();
            break;
        }
        if(!isMusicPlaying() || curMusicPlaying!=(int)currentState)
        {
            startMusic((int)currentState);
        }
    }
    return 0;
}
