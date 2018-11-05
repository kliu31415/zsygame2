vector<int> menu_select; //which box is selected in the menu
int indiv_selected; //used to denote player number in controls
enum class menuScreen{mainMenu, options, versionInfo, video, gameSettings};
vector<menuScreen> cur_menu; //which menu is currently being used
int MENU_W_OFFSET, MENU_H_OFFSET, MENU_W;
void nextMenu(menuScreen x) //going forward (Ex. main->options)
{
    menu_select.push_back(0);
    cur_menu.push_back(x);
}
void prevMenu() //going back (Ex. options->main)
{
    menu_select.pop_back();
    cur_menu.pop_back();
}
struct menu_option
{
    virtual void inc() = 0;
    virtual void dec() = 0;
    virtual void enter() = 0;
    virtual void drawInfo(int x, int y, int s) = 0;
};
template<class T> struct T_menu_option: public menu_option
{
    string name;
    T minVal, maxVal, baseVal, interval;
    T *curVal;
    double mult; //sometimes we should multiply the actual value to display a more friendly user value
    T_menu_option(string n, T *c, T basev, T minv, T maxv, T i, double m)
    {
        name = n;
        curVal = c;
        minVal = minv;
        maxVal = maxv;
        baseVal = basev;
        mult = m;
        interval = i;
    }
    void inc()
    {
        if(interval > 0)
            *curVal = min((T)(*curVal + interval), maxVal);
        else *curVal = max((T)(*curVal + interval), minVal);
    }
    void dec()
    {
        if(interval > 0)
            *curVal = max((T)(*curVal - interval), minVal);
        else *curVal = min((T)(*curVal - interval), maxVal);
    }
    void enter()
    {
        *curVal = baseVal;
    }
    void drawInfo(int x, int y, int s)
    {
        drawText(name, x, y, s);
        drawText(round(mult * (*curVal), 2), MENU_W_OFFSET + MENU_W + settings::WINDOW_W*0.02, y, s);
    }
};
template<> struct T_menu_option<bool>: public menu_option
{
    string name;
    bool *curVal;
    T_menu_option(string n, bool *c)
    {
        name = n;
        curVal = c;
    }
    void inc(){}
    void dec(){}
    void enter()
    {
        *curVal = !*curVal;
    }
    void drawInfo(int x, int y, int s)
    {
        drawText(name, x, y, s);
        if(*curVal)
            drawText(to_str("TRUE"), MENU_W_OFFSET + MENU_W + settings::WINDOW_W*0.02, y, s);
        else drawText(to_str("FALSE"), MENU_W_OFFSET + MENU_W + settings::WINDOW_W*0.02, y, s);
    }
};
template<> struct T_menu_option<string>: public menu_option
{
    string name;
    vector<string> vals;
    int *curVal, baseVal, minVal, maxVal, offset;
    T_menu_option(string n, int *c, int basev, int minv, int maxv, vector<string> v)
    {
        name = n;
        curVal = c;
        minVal = minv;
        maxVal = maxv;
        baseVal = basev;
        vals = v;
    }
    void inc()
    {
        *curVal = min(*curVal + 1, maxVal);
    }
    void dec()
    {
        *curVal = max(*curVal - 1, minVal);
    }
    void enter()
    {
        *curVal = baseVal;
    }
    void drawInfo(int x, int y, int s)
    {
        drawText(name, x, y, s);
        drawText(vals[*curVal - minVal], MENU_W_OFFSET + MENU_W + settings::WINDOW_W*0.02, y, s);
    }
};
struct menuOptionNavigation{}; //dummy
template<> struct T_menu_option<menuOptionNavigation>: public menu_option
{
    string name;
    void (*func)();
    T_menu_option(string n, void (*f)())
    {
        name = n;
        func = f;
    }
    void inc(){}
    void dec(){}
    void enter()
    {
        func();
    }
    void drawInfo(int x, int y, int s)
    {
        drawText(name, x, y, s);
    }
};
struct menu_screen
{
    string title;
    vector<string> info; //useful info that is displayed at the bottom
    vector<menu_option*> options;
    void render()
    {
        using namespace settings;
        int H = WINDOW_H, W = WINDOW_W;
        renderClear(120, 120, 120);
        drawText(title, MENU_W_OFFSET, H*0.05, H*0.1);
        fillRect(MENU_W_OFFSET - W*0.01, H*0.14, MENU_W + WINDOW_W*0.02, H*(0.02 + 0.05*(options.size()+1)), 150, 150, 150); //outer box
        fillRect(MENU_W_OFFSET, H*0.15, MENU_W, H*0.05*(options.size()+1), 200, 200, 200); //inner box
        fillRect(MENU_W_OFFSET, H*(0.15 + 0.05*menu_select.back()), MENU_W, H*0.05 + 1, 200, 150, 100); //highlight the selected option
        setColor(0, 0, 0);
        double hOffset = H*0.15;
        for(auto &i: options)
        {
            i->drawInfo(MENU_W_OFFSET, hOffset, H*0.05);
            hOffset += H*0.05;
            drawLine(MENU_W_OFFSET, hOffset, MENU_W_OFFSET + MENU_W, hOffset);
        }
        if(title == "MAIN MENU")
            drawText("QUIT", MENU_W_OFFSET, H*(0.15 + 0.05*options.size()), H*0.05); //you can't go "back" from the main menu
        else drawText("BACK", MENU_W_OFFSET, H*(0.15 + 0.05*options.size()), H*0.05);
        for(int i=0; i<info.size(); i++)
            drawText(info[i], W*0.5 - (H*0.0125*info[i].size()), H*(0.26 + 0.05*(options.size() + i)), H*0.05);
    }
    void inc(int pos)
    {
        options[pos]->inc();
    }
    void dec(int pos)
    {
        options[pos]->dec();
    }
    void enter(int pos)
    {
        options[pos]->enter();
    }
};
menu_screen gameSettingsMenu, videoMenu, mainMenu, optionsMenu;
template<class T> void addMenuOption(vector<menu_option*> &o, string n, T *c, T basev, T minv, T maxv, T i, double m=1)
{
    o.push_back(new T_menu_option<T>(n, c, basev, minv, maxv, i, m));
}
void addBoolMenuOption(vector<menu_option*> &o, string n, bool *c)
{
    o.push_back(new T_menu_option<bool>(n, c));
}
//minv is for stuff that can be negative like font quality
void addStringMenuOption(vector<menu_option*> &o, string n, int *c, int basev, int minv, int maxv, ...)
{
    vector<string> vals;
    va_list v;
    va_start(v, maxv-minv+1);
    for(int i=0; i<=maxv-minv; i++)
        vals.push_back(va_arg(v, const char*));
    o.push_back(new T_menu_option<string>(n, c, basev, minv, maxv, vals));
}
void addNavigationMenuOption(vector<menu_option*> &o, string n, void (*f)())
{
    o.push_back(new T_menu_option<menuOptionNavigation>(n, f));
}
