#include "menu_base.h"
namespace versionInfo
{
    string compilerName;
    string compilerVersion;
    string operatingSystem;
    string sdl2CompileVersion;
    string sdl2ImageCompileVersion;
    string sdl2TTFCompileVersion;
    string sdl2MixCompileVersion;
    string fontName;
    string gameVersion;
    string cppVersion;
    string displayDPI;
    string cpuThreads;
    string RAM;
    string audioDriver;
    string videoDriver;
    string displayName;
    string displayIndex;
    string displayResolution;
    void init()
    {
        //get compiler name and version
        #ifdef __MINGW64__
        compilerName = "MinGW64";
        compilerVersion = __VERSION__;
        #elif __MINGW32__
        compilerName = "MinGW32";
        compilerVersion = __VERSION__;
        #elif _MSC_VER
        compilerName = "MSVC";
        compilerVersion = to_str(_MSC_VER);
        #elif __GCC__
        compilerName = "GCC";
        compilerVersion = __VERSION__;
        #else
        compilerName = "(Unknown Compiler)";
        compilerVersion = "(Unknown Version)";
        #endif
        operatingSystem = SDL_GetPlatform();
        //get SDL2 version
        SDL_version cV;
        SDL_VERSION(&cV);
        sdl2CompileVersion = to_str((int)cV.major) + "." + to_str((int)cV.minor) + "." + to_str((int)cV.patch);
        SDL_IMAGE_VERSION(&cV);
        sdl2ImageCompileVersion = to_str((int)cV.major) + "." + to_str((int)cV.minor) + "." + to_str((int)cV.patch);
        TTF_VERSION(&cV);
        sdl2TTFCompileVersion = to_str((int)cV.major) + "." + to_str((int)cV.minor) + "." + to_str((int)cV.patch);
        MIX_VERSION(&cV);
        sdl2MixCompileVersion = to_str((int)cV.major) + "." + to_str((int)cV.minor) + "." + to_str((int)cV.patch);
        //get font name
        fontName = to_str(TTF_FontFaceFamilyName(font[0])) + " " + to_str(TTF_FontFaceStyleName(font[0]));
        //get game version
        gameVersion = __GAME__VERSION__;
        //get C++ version
        cppVersion = to_str((int)__cplusplus);
        //get DPI
        int dispIndex = SDL_GetWindowDisplayIndex(window);
        displayIndex = to_str(dispIndex);
        float dpi;
        SDL_GetDisplayDPI(dispIndex, &dpi, &dpi, &dpi);
        displayDPI = to_str((int)dpi);
        //get #cpu threads
        cpuThreads = to_str(thread::hardware_concurrency());
        RAM = to_str(SDL_GetSystemRAM()) + "MB";
        audioDriver = SDL_GetCurrentAudioDriver();
        videoDriver = SDL_GetCurrentVideoDriver();
        displayName = SDL_GetDisplayName(dispIndex);
        rect dispBounds;
        SDL_GetDisplayBounds(dispIndex, &dispBounds);
        displayResolution = to_str(dispBounds.w) + "x" + to_str(dispBounds.h);
    }
    void showVInfo(string s, bool clr = 0)
    {
        using namespace settings;
        static int hStart;
        if(clr)
            hStart = WINDOW_H*0.22;
        drawText(s, WINDOW_W*0.1, hStart, BASE_FONT_SIZE*2/3);
        hStart += BASE_FONT_SIZE*2/3;
    }
}
void operateVersionInfoMenu()
{
    const static int MENU_OPTIONS = 1;
    int H = settings::WINDOW_H, W = settings::WINDOW_W;
    renderClear(120, 120, 120);
    drawText("INFO", MENU_W_OFFSET, H*0.05, H*0.1);
    fillRect(MENU_W_OFFSET - W*0.01, H*0.14, MENU_W + W*0.02, H*(0.02 + 0.05*MENU_OPTIONS), 150, 150, 150); //outer box
    fillRect(MENU_W_OFFSET, H*0.15, MENU_W, H*0.05*MENU_OPTIONS, 200, 200, 200); //inner box
    fillRect(MENU_W_OFFSET, H*(0.15 + 0.05*menu_select.back()), MENU_W, H*0.05 + 1, 200, 150, 100); //highlight the selected option
    setColor(0, 0, 0);
    for(int i=1; i<MENU_OPTIONS; i++)
        drawLine(MENU_W, H*(0.2 + i*0.05), W*0.6, H*(0.2 + i*0.05));
    drawText("BACK", MENU_W_OFFSET, H*0.15, H*0.05); //option 0
    //show version info
    using namespace versionInfo;
    int baseFontSize = settings::WINDOW_H/30; //standard font size
    showVInfo("Date Compiled: " + to_str(__DATE__) + " " + to_str(__TIME__), true);
    showVInfo("Compiler: " + compilerName + " v" + compilerVersion);
    showVInfo("C++ Version: " + cppVersion);
    showVInfo("Operating System: " + operatingSystem);
    showVInfo("SDL2 version: " + sdl2CompileVersion);
    showVInfo("SDL2_image version: " + sdl2ImageCompileVersion);
    showVInfo("SDL2_ttf version: " + sdl2TTFCompileVersion);
    showVInfo("SDL2_mixer version: " + sdl2MixCompileVersion);
    showVInfo("Font: " + fontName);
    showVInfo("CPU threads: " + cpuThreads);
    showVInfo("RAM: " + RAM);
    showVInfo("Audio Driver: " + audioDriver);
    showVInfo("Video Driver: " + videoDriver);
    showVInfo("Display Index: " + displayIndex);
    showVInfo("Display Name: " + displayName);
    showVInfo("Display DPI: " + displayDPI);
    showVInfo("Display Resolution: " + displayResolution);
    showVInfo("Game Version: " + gameVersion);
    showVInfo("Game Creator: Kevin Liu");
    showVInfo("Time in game: " + seconds_to_str(getTicks() / 1000));
    while(SDL_PollEvent(&input))
    {
        switch(input.type)
        {
        case SDL_QUIT:
            exitGame();
            return;
        case SDL_KEYDOWN:
            switch(input.key.keysym.sym)
            {
            case SDLK_RETURN:
                prevMenu();
                break;
            case SDLK_ESCAPE:
                prevMenu();
                break;
            }
            break;
        }
    }
}
void initMenu()
{
    using namespace settings;
    versionInfo::init();
    currentState = gameState::menu;
    menu_select.clear();
    menu_select.push_back(0);
    cur_menu.clear();
    cur_menu.push_back(menuScreen::mainMenu);
    static bool alreadyInit = false;
    if(!alreadyInit)
    {
    MENU_W_OFFSET = WINDOW_W * 0.3;
    MENU_H_OFFSET = WINDOW_H * 0.1;
    MENU_W = WINDOW_W * 0.4;
    alreadyInit =true;
    {//main menu
    mainMenu.title = "MAIN MENU";
    vector<menu_option*> &o = mainMenu.options;
    addNavigationMenuOption(o, "PLAY", initNewGame);
    addNavigationMenuOption(o, "OPTIONS", []()->void {nextMenu(menuScreen::options);});
    addNavigationMenuOption(o, "INFO", []()->void {nextMenu(menuScreen::versionInfo);});
    }
    {//options
    optionsMenu.title = "OPTIONS";
    vector<menu_option*> &o = optionsMenu.options;
    addNavigationMenuOption(o, "GAME", []()->void {nextMenu(menuScreen::gameSettings);});
    addNavigationMenuOption(o, "VIDEO/AUDIO", []()->void {nextMenu(menuScreen::video);});
    }
    {//game settings menu
    gameSettingsMenu.title = "SETTINGS";
    gameSettingsMenu.info.push_back("PRESS ENTER TO RESET A FIELD");
    vector<menu_option*> &o = gameSettingsMenu.options;
    addMenuOption(o, "AI Randomness", &AI_Randomness, 0.0, 0.0, 100.0, 5.0);
    addMenuOption(o, "AI Point Affinity", &AI_PointAffinity, 1.0, 0.0, 10.0, 0.2);
    addMenuOption(o, "AI Min Turn Length", &AI_MinTurnLength, 2000, 0, 10000, 200);
    addMenuOption(o, "AI Bomb Affinity", &AI_BombAffinity, 0.0, -100.0, 100.0, 5.0);
    addMenuOption(o, "AI Straight Flush Affinity", &AI_StraightFlushAffinity, 0.0, -100.0, 100.0, 5.0);
    addMenuOption(o, "AI Single Affinity", &AI_SingleAffinity, 0.0, -100.0, 100.0, 5.0);
    addMenuOption(o, "AI Double Affinity", &AI_DoubleAffinity, 0.0, -100.0, 100.0, 5.0);
    addMenuOption(o, "AI Triple Affinity", &AI_TripleAffinity, 0.0, -100.0, 100.0, 5.0);
    addMenuOption(o, "AI Full House Affinity", &AI_FullhouseAffinity, 0.0, -100.0, 100.0, 5.0);
    addMenuOption(o, "AI Straight Affinity", &AI_StraightAffinity, 0.0, -100.0, 100.0, 5.0);
    addMenuOption(o, "Points To Win", &PointsToWin, 1000, 100, 2000, 100);
    }
    {//video menu
    videoMenu.title = "VIDEO/AUDIO";
    videoMenu.info.push_back("SOME CHANGES MAY REQUIRE RESTARTING");
    videoMenu.info.push_back("5:3 WINDOW_W/WINDOW_H RATIO IS OPTIMAL");
    vector<menu_option*> &o = videoMenu.options;
    addBoolMenuOption(o, "LOW TEXTURE QUALITY", &lowTextureQuality);
    addBoolMenuOption(o, "VSYNC", &vsync);
    addBoolMenuOption(o, "ACCELERATED RENDERER", &acceleratedRenderer);
    addBoolMenuOption(o, "FULLSCREEN", &IS_FULLSCREEN);
    addBoolMenuOption(o, "SHOW FPS", &showFPS);
    addMenuOption(o, "FPS CAP", &FPS_CAP, 300, 1, 300, 1);
    addMenuOption(o, "WINDOW WIDTH", &FOUT_WINDOW_W, 2500, 100, 4000, 10);
    addMenuOption(o, "WINDOW HEIGHT", &FOUT_WINDOW_H, 1500, 50, 2400, 10);
    //For some reason more and more text doesn't show as fontQuality gets higher and higher (RAM doesn't seem to be the issue either)
    addStringMenuOption(o, "FONT QUALITY", &fontQuality, 1, -2, 2, "Very Low", "Low", "Medium", "High", "Very High");
    addStringMenuOption(o, "RENDER SCALE QUALITY", &renderScaleQuality, 0, 0, 2, "Nearest Pixel", "Linear", "Anisotropic");
    addMenuOption(o, "TEXT TEXTURE CACHE TIME", &TEXT_TEXTURE_CACHE_TIME, 1100, 0, 5000, 100);
    addMenuOption(o, "MUSIC VOLUME", &volume, MIX_MAX_VOLUME, 0, MIX_MAX_VOLUME, 2);
    }
    }
}
void operateMenuScreen(menu_screen &x)
{
    using namespace settings;
    x.render();
    int &m = menu_select.back();
    int sz = x.options.size();
    while(SDL_PollEvent(&input))
    {
        switch(input.type)
        {
        case SDL_QUIT:
            exitGame();
            return;
        case SDL_KEYDOWN:
            switch(input.key.keysym.sym)
            {
            case SDLK_UP:
                m += sz;
                m %= sz+1;
                break;
            case SDLK_DOWN:
                m++;
                m %= sz+1;
                break;
            case SDLK_LEFT:
                if(m != sz)
                    x.dec(m);
                break;
            case SDLK_RIGHT:
                if(m != sz)
                    x.inc(m);
                break;
            case SDLK_RETURN:
                if(m != sz)
                    x.enter(m);
                else prevMenu();
                break;
            case SDLK_ESCAPE:
                prevMenu();
                break;
            }
            break;
        }
    }
}
void operateMenu()
{
    switch(cur_menu.back())
    {
    case menuScreen::mainMenu:
        operateMenuScreen(mainMenu);
        break;
    case menuScreen::options:
        operateMenuScreen(optionsMenu);
        break;
    case menuScreen::versionInfo:
        operateVersionInfoMenu();
        break;
    case menuScreen::video:
        operateMenuScreen(videoMenu);
        setVolume(settings::volume); //idk why but I just put the volume in the "video" settings
        break;
    case menuScreen::gameSettings:
        operateMenuScreen(gameSettingsMenu);
        break;
    }
    updateScreen();
}
