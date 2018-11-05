//A simple SDL2 wrapper by Kevin Liu
#include <string>
#include <SDL2/SDL.h>
typedef SDL_Texture texture;
typedef SDL_Rect rect;
extern SDL_Event input;
namespace sdl_settings
{
    extern bool lowTextureQuality, vsync, acceleratedRenderer;
    extern int WINDOW_W, WINDOW_H, FOUT_WINDOW_W, FOUT_WINDOW_H; //FOUT allows WINDOW_W and WINDOW_H to be saved for a restart
    extern int volume, renderScaleQuality, fontQuality;
    extern double BASE_CARD_H_RATIO, SPRITE_SCALE;
    extern bool showFPS, IS_FULLSCREEN; //overrides WINDOW_W and WINDOW_H
    extern int FPS_CAP; //FPS cap (300 is essentially uncapped)
    extern int TEXT_TEXTURE_CACHE_TIME;
    /**
    Reads sdl_settings variables from a file
    */
    void output_config();
    /**
    Writes sdl_settings variables from a file
    */
    void load_config();
}
/**
This prints a string to stdout and supports multi-threaded printing by using mutexes
*/
void print(std::string s);
/**
This prints a string and appends a newline to stdout and supports multi-threaded printing by using mutexes
*/
void println(std::string s);
/**
Converts something to a string
*/
template<class T> std::string to_str(T x);
/**
Converts an integer number of seconds to time in the form H:MM:SS
*/
std::string seconds_to_str(int t);
/**
Generates a random integer from 0 to RANDUZ_MAX using std::mt19937
*/
int randuz();
/**
Generates a random integer from 0 to m-1
*/
int randuzm(int m);
/**
Returns a random float from 0 to 1
*/
double randf();
/**
Returns a random integer from v1 to v2
*/
int randz(int v1, int v2);
/**
Rounds a double to the specified number of places (0 to 9)
*/
double round(double x, int places);
/**
Returns the number of milliseconds since initSDL was called
*/
int getTicks();
/**
Sets the volume of SDL_Mixer
*/
void setVolume(int vol);
/**
Initializes SDL
*/
void initSDL();
/**
Equivalent to SDL_SetRenderDrawColor
*/
inline void setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Equivalent to SDL_RenderClear
*/
inline void renderClear();
/**
Equivalent to SDL_RenderClear
*/
inline void renderClear(uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Equivalent to SDL_RenderCopy
*/
inline void renderCopy(texture *t, rect *dst);
/**
Equivalent to SDL_RenderCopy
*/
inline void renderCopy(texture *t, rect *src, rect *dst);
/**
Equivalent to SDL_RenderCopy
*/
inline void renderCopy(texture *t, int x, int y, int w, int h);
/**
Equivalent to SDL_RenderCopyEx
*/
inline void renderCopyEx(texture *t, int x, int y, int w, int h, double rot, SDL_RendererFlip f = SDL_FLIP_NONE);
//a text texture cache greatly speeds up stuff because we don't have to create the texture every time
struct text_info
{
    std::string s;
    uint8_t r, g, b;
    int sz;
    text_info(std::string ss, int size, uint8_t r1, uint8_t g1, uint8_t b1);
    bool operator<(text_info x) const;
};
/**
Draws unwrapped text on the window. The texture is cached for TEXT_TEXTURE_CACHE_TIME (1100ms by default).
*/
template<class T> void drawText(T txt, int x, int y, int s, uint8_t r=0, uint8_t g=0, uint8_t b=0);
/**
Draws wrapped text on the window. The texture is cached for TEXT_TEXTURE_CACHE_TIME.
*/
template<class T> int drawMultilineText(T txt, int x, int y, int w, int s, uint8_t r=0, uint8_t g=0, uint8_t b=0);
/**
Returns how many times a given text will be wrapped if it is drawn
*/
template<class T> int multilineTextLength(T txt, int w, int s);
/**
Displays a loading screen
*/
void showLoadingScreen();
/**
Equivalent to SDL_RenderFillRect
*/
void fillRect(SDL_Rect *x);
/**
Equivalent to SDL_RenderFillRect
*/
void fillRect(SDL_Rect *x, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Equivalent to SDL_RenderFillRect
*/
void fillRect(int x, int y, int w, int h);
/**
Equivalent to SDL_RenderFillRect
*/
void fillRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Equivalent to SDL_RenderDrawRect
*/
void drawRect(int x, int y, int w, int h);
/**
Equivalent to SDL_RenderDrawRect
*/
void drawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Loads a texture from an image file and color keys it
*/
SDL_Texture *loadTexture(const char *name, uint8_t r, uint8_t g, uint8_t b);
/**
Loads a texture from an image file
*/
SDL_Texture *loadTexture(const char *name);
/**
Checks if two rectangles intersect
*/
inline bool rectsIntersect(rect a, rect b);
/**
Equivalent to SDL_RenderDrawLine
*/
inline void drawLine(int x1, int y1, int x2, int y2);
/**
Equivalent to SDL_RenderDrawLine
*/
inline void drawLine(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Equivalent to SDL_RenderDrawPoint
*/
inline void drawPoint(int x, int y);
/**
Equivalent to SDL_RenderDrawPoint
*/
inline void drawPoint(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a=0);
/**
Draws a filled circle
*/
void fillCircle(int x, int y, int r);
/**
Draws a filled circle
*/
void fillCircle(int x, int y, int rad, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Draws an unfilled circle
*/
void drawCircle(int x, int y, int r);
/**
Draws an unfilled circle
*/
void drawCircle(int x, int y, int rad, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Checks if the mouse is in a given rectangle
*/
bool mouseInRect(int x, int y, int w, int h);
/**
Checks if the mouse is in a given rectangle
*/
bool mouseInRect(SDL_Rect *x);
/**
Returns the x coordinate of the mouse. The mouse state is updated during the updateScreen() function.
*/
int getMouseX();
/**
Returns the y coordinate of the mouse. The mouse state is updated during the updateScreen() function.
*/
int getMouseY();
/**
Updates the screen and performs some other functions. This function is called to advance to the next frame.
*/
void updateScreen();
