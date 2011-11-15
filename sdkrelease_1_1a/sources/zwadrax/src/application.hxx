#ifndef _APPLICATION_HXX_
#define _APPLICATION_HXX_

#define AUTO_SAVE_NAME  "_auto_save_"

template <typename type>
class deferred_type
{
private:
    std::list<type> m_obj;
public:
    type & operator =(const type &obj)
    {
        m_obj.clear();
        m_obj.push_back(obj);
        return m_obj.front();
    }
    void clear(void)
    {
        m_obj.clear();
    }
    type & operator ()(void)
    {
        assert(!m_obj.empty());
        return m_obj.front();
    }
    const type & operator ()(void) const
    {
        assert(!m_obj.empty());
        return m_obj.front();
    }
    bool empty(void) const
    {
        return m_obj.empty();
    }
};

class application_type
{
private:
    // control keys
    enum evt_type {
        NO_EVT      = game_type::NO_KEY,
        SWITCH      = game_type::SWITCH,
        LEFT        = game_type::LEFT,
        RIGHT       = game_type::RIGHT,
        UP          = game_type::UP,
        DOWN        = game_type::DOWN,
        ACTIVATE    = game_type::ACTIVATE,
        MENU3X3,
        MENU,
        EXIT,
        SHIFTFIRST,
        SHIFTNEXT,
        SHIFTCENTER
    };
    // max items in menu
    enum { ITEMLIMIT = 1024 };
    // menu indexes
    enum {
        MENU_NEXT,
        MENU_RESTART,
        MENU_LOADLEVEL,
        MENU_EDITLEVEL,
        MENU_SAVELEVEL,
        MENU_LOADSOLUTION,
        MENU_SAVESOLUTION,
        MENU_LOADLEVELSET,
        MENU_EXIT,
        MENU_LEVELSET = ITEMLIMIT,
        MENU_LEVEL = MENU_LEVELSET + ITEMLIMIT,
        MENU_SOLUTION = MENU_LEVEL + ITEMLIMIT
    };

    typedef set<string> nameset_type;

    // game serializer
    serializer_type m_serializer;
    // name of current levelset
    string m_levelset;
    // name of current level
    string m_level;
    // current game (0 or 1 instances)
    deferred_type<game_type> m_game;
    // picture combiner
    picstore_type m_picstore;
    // shiftable window support
    viewport_type m_viewport;

    // names of passed levels in current levelset
    nameset_type m_passed;
    // current content of main menu
    vector<imenu> m_mainmenu;
    // current content of dynamic menu
    vector<imenu> m_dynmenu;
    // names of all dynmenu items
    stringlist_type m_dynmenustr;
    // current content of keyboard input
    vector<char> m_buffer;
    // last selected main menu index
    int m_lastmenu;
    // drawing queue flag
    bool m_drawing;
    // control rectangles
    irect m_padrect, m_sightrect;
    // map shifting start point
    coord_type m_hold;

    // output main menu
    void menumain(void)
    {
        imenu *mainptr, *levelptr, *solutionptr;
        int num;

        // previous game was won (next non-passed can be selected)
        bool gamewon = m_game.empty() || m_game().iswon();
        // game or edit mode is on
        bool ingame = !m_game.empty() && !m_game().isedit();
        bool inedit = !m_game.empty() && m_game().isedit();

        m_mainmenu.clear();
        m_mainmenu.reserve(ITEMLIMIT);

        // level submenu
        {
            imenu menu[] = {
                { ITEM_ACTIVE, MENU_LOADLEVEL, "load", NULL },
                { ITEM_SEPARATOR, 0, NULL, NULL },
                { ingame ? ITEM_ACTIVE : ITEM_INACTIVE, MENU_EDITLEVEL, "edit", NULL },
                { inedit ? ITEM_ACTIVE : ITEM_INACTIVE, MENU_SAVELEVEL, "save as", NULL },
                { 0, 0, NULL, NULL }
            };
            num = sizeof(menu) / sizeof(menu[0]);
            m_mainmenu.insert(m_mainmenu.end(), &menu[0], &menu[0] + num);
            levelptr = &m_mainmenu.front() + m_mainmenu.size() - num;
        };

        // solution submenu
        {
            imenu menu[] = {
                { ITEM_ACTIVE, MENU_LOADSOLUTION, "load", NULL },
                { ITEM_ACTIVE, MENU_SAVESOLUTION, "save as", NULL },
                { 0, 0, NULL, NULL }
            };
            num = sizeof(menu) / sizeof(menu[0]);
            m_mainmenu.insert(m_mainmenu.end(), &menu[0], &menu[0] + num);
            solutionptr = &m_mainmenu.front() + m_mainmenu.size() - num;
        };

        // main menu
        {
            imenu menu[] = {
                { ITEM_HEADER, 0, "menu", NULL },
                { gamewon ? ITEM_ACTIVE : ITEM_INACTIVE, MENU_NEXT, "next level", NULL },
                { ingame ? ITEM_ACTIVE : ITEM_INACTIVE, MENU_RESTART, "restart", NULL },
                { ITEM_SEPARATOR, 0, NULL, NULL },
                { ITEM_SUBMENU, 0, "level", levelptr },
                { ingame ? ITEM_SUBMENU : ITEM_INACTIVE, 0, "solution", solutionptr },
                { ITEM_ACTIVE, MENU_LOADLEVELSET, "level set", NULL },
                { ITEM_SEPARATOR, 0, NULL, NULL },
                { ITEM_ACTIVE, MENU_EXIT, "exit", NULL },
                { 0, 0, NULL, NULL }
            };
            num = sizeof(menu) / sizeof(menu[0]);
            m_mainmenu.insert(m_mainmenu.end(), &menu[0], &menu[0] + num);
            mainptr = &m_mainmenu.front() + m_mainmenu.size() - num;
        };

        // we need it for assurance that there were no reallocations
        assert(m_mainmenu.size() < ITEMLIMIT);

        suspendqueue();
        ::OpenMenu(
            mainptr,
            0,
            ::ScreenWidth() / 4,
            ::ScreenHeight() / 16,
            menu_handler);
    }

    // output dynamic menu (form it from strings and starting index)
    void menudynamic(
        const stringlist_type &strsrc,
        const stringlist_type &marked,
        int startidx)
    {
        m_dynmenustr = strsrc;
        m_dynmenu.clear();
        if (!m_dynmenustr.empty())
        {
            nameset_type sel(marked.begin(), marked.end());

            imenu first = { ITEM_HEADER, 0, "choice", NULL };
            m_dynmenu.push_back(first);

            int idx = startidx;
            stringlist_type::const_iterator it;
            for (it = m_dynmenustr.begin(); it != m_dynmenustr.end(); it++)
            {
                imenu im = {
                    (sel.find(*it) == sel.end()) ? ITEM_ACTIVE : ITEM_BULLET,
                    idx, (char *) it->c_str(), NULL
                };
                m_dynmenu.push_back(im);
                if (++idx >= startidx + ITEMLIMIT)
                    break;
            };
            imenu last = { 0, 0, NULL, NULL };
            m_dynmenu.push_back(last);

            suspendqueue();
            ::OpenMenu(&m_dynmenu.front(),
                0,
                ::ScreenWidth() / 2,
                ::ScreenHeight() / 8,
                menu_handler);
        }
        else
        {
			message("there are no accessible items");
            menumain();
        };
    }

    // output shifting menu
    void menushift(void)
    {
        extern const ibitmap move3x3;
        static const char *strings3x3[] = {
            "move left-up",
            "move up",
            "move up-right",
            "move left",
            "rotate",
            "move right",
            "move left-down",
            "move down",
            "move down-right"
        };
        suspendqueue();
        ::OpenMenu3x3(&move3x3, strings3x3, menu3x3_handler);
    }

    // output message to user
    void message(const string &msg) const
    {
        ::Message(ICON_INFORMATION, "message", (char *) msg.c_str(), 3000);
    }

    // status bar height
    int barsize(void) const
    {
        return ::ScreenHeight() / 8;
    }
    // max width and height of viewport for current screen
    int maxwidth(void) const
    {
        return ::ScreenWidth() / m_picstore.width();
    }
    int maxheight(void) const
    {
        return (::ScreenHeight() - barsize()) / m_picstore.height();
    }
    // start position (in pixels) of game window
    int posx(void) const
    {
        return (::ScreenWidth() - m_viewport.width() * m_picstore.width()) / 2;
    }
    int posy(void) const
    {
        return (::ScreenHeight() - barsize() - m_viewport.height() * m_picstore.height()) / 2;
    }

    // start game with corresponding solution or editing level
    void start(
        bool game,
        const string &level,
        const string &solstr = "",
        bool play = true)
    {
        DPRINTF("BEGIN %s %s", level.c_str(), solstr.c_str());

        m_level = level;
        ::ShowHourglass();
        if (game)
        {
            // load solution
            game_type::sequence_type sequence;
            if (!solstr.empty())
                sequence = m_serializer.load_solution(m_levelset, level, solstr);
            // start new game in game mode
            m_game = game_type(m_serializer.load_level(m_levelset, m_level), sequence, play);
        }
        else
        {
            // start new game in edit mode
            m_game = game_type(m_serializer.load_level(m_levelset, m_level));
        };
        ::HideHourglass();

        // init viewport sizes
        m_viewport = viewport_type(
            m_game().width(), m_game().height(),
            maxwidth(), maxheight());
        fullredraw();
        drawqueue();

        DPRINTF("END %s %s", level.c_str(), solstr.c_str());
    }

    // store levelset state
    void store(void)
    {
        // store config
        stringmap_type cfg;
        // store level or solution
        if (!m_game.empty())
        {
            cfg["mode"] = m_game().isedit() ? "edit" : "game";
            if (m_game().isedit())
            {
                // store level
                m_level = AUTO_SAVE_NAME;
                m_serializer.save_level(
                    m_game().data(),
                    m_levelset,
                    m_level);
            }
            else
            {
                // store solution
                m_serializer.save_solution(
                    m_game().sequence(),
                    m_levelset,
                    m_level,
                    AUTO_SAVE_NAME);
            };
            cfg["level"] = m_level;
        };
        // passed levels
        cfg["passed"] = serializer_type::join(
            stringlist_type(m_passed.begin(), m_passed.end()));

        m_serializer.save_config(cfg, m_levelset);

        // clear game
        m_game.clear();
        m_level = "";
        m_passed.clear();
    }
    // restore levelset state
    void restore(void)
    {
        stringmap_type cfg = m_serializer.load_config(m_levelset);
        m_serializer.erase_config(m_levelset);

        // passed levels
        stringlist_type passed = serializer_type::split(cfg["passed"]);
        m_passed = nameset_type(passed.begin(), passed.end());

        // if there is no stored name
        m_level = cfg["level"];
        if (m_level.empty())
        {
            // redraw screen
            fullredraw();
            // try to load next non-passed level
            handlemenu(MENU_NEXT);
        }
        else
        {
            // start level with stored name
            if (cfg["mode"] == "edit")
            {
                start(false, AUTO_SAVE_NAME);
                m_serializer.erase_level(m_levelset, AUTO_SAVE_NAME);
            }
            else
            {
                start(true, m_level, AUTO_SAVE_NAME, false);
                m_serializer.erase_solution(m_levelset, m_level, AUTO_SAVE_NAME);
            };
        };
    }

    void fullredraw(void)
    {
        // prepare game area and status bar
        ::ClearScreen();

        // fill area around a viewport
        // (black-white pixels in checker order)
        for (int x = 0; x < ::ScreenWidth(); x++)
            for (int y = 0; y < ::ScreenHeight() - barsize(); y++)
                ::DrawPixel(x, y, (((x + y) & 1) == 0) ? BLACK : WHITE);

        // change window size and repaint content
        if (!m_game.empty())
        {
            m_viewport.resize(maxwidth(), maxheight());
            drawone(m_viewport.content());
        };
        // repaint status bar
        drawbar();

        ::FullUpdate();
        /*
        // we must uncomment it, if we don't need a FullUpdate
        ::PartialUpdateBW(
            0, 0,
            ::ScreenWidth(), ::ScreenHeight() - barsize());
        */
    }

    // status bar repaint
    void drawbar(void)
    {
        enum { KEYS, VIEW, STAT, ALL };
        irect fld[] = {
            // keys - 3/8 of screen width
            {
                0,
                ::ScreenHeight() - barsize(),
                ::ScreenWidth() * 3 / 8,
                barsize()
            },
            // viewport - 1/4 of screen width
            {
                ::ScreenWidth() * 3 / 8,
                ::ScreenHeight() - barsize(),
                ::ScreenWidth() / 4,
                barsize()
            },
            // status - 3/8 of screen width
            {
                ::ScreenWidth() * 5 / 8,
                ::ScreenHeight() - barsize(),
                ::ScreenWidth() * 3 / 8,
                barsize()
            },
            // whole bar
            {
                0,
                ::ScreenHeight() - barsize(),
                ::ScreenWidth(),
                barsize()
            }
        };


        // clean previous bar content
        ::FillArea(fld[ALL].x, fld[ALL].y, fld[ALL].w, fld[ALL].h, WHITE);

        // draw viewport position
        ::DrawRect(fld[VIEW].x, fld[VIEW].y, fld[VIEW].w, fld[VIEW].h, BLACK);
        double rx, ry, rw, rh;
        m_viewport.ratio(rx, ry, rw, rh);
        int vx = int(rx * fld[VIEW].w), vy = int(ry * fld[VIEW].h);
        int vw = int(rw * fld[VIEW].w), vh = int(rh * fld[VIEW].h);
        ::DrawRect(fld[VIEW].x + vx, fld[VIEW].y + vy, vw, vh, BLACK);
        int hl = 3;
        // draw hairlines
        ::DrawRect(
            fld[VIEW].x + vx + vw / 2 - hl / 2, fld[VIEW].y + vy,
            hl, vh / 8,
            BLACK);
        ::DrawRect(
            fld[VIEW].x + vx + vw / 2 - hl / 2, fld[VIEW].y + vy + vh - vh / 8,
            hl, vh / 8,
            BLACK);
        ::DrawRect(
            fld[VIEW].x + vx, fld[VIEW].y + vy + vh / 2 - hl / 2,
            vw / 8, hl,
            BLACK);
        ::DrawRect(
            fld[VIEW].x + vx + vw - vw / 8, fld[VIEW].y + vy + vh / 2 - hl / 2,
            vw / 8, hl,
            BLACK);
        m_sightrect = fld[VIEW];

        // draw control pad
        extern const ibitmap controlpad;
        irect padrect = {
            fld[KEYS].x + (fld[KEYS].w - controlpad.width) / 2,
            fld[KEYS].y + (fld[KEYS].h - controlpad.height) / 2,
            controlpad.width,
            controlpad.height
        };
        m_padrect = padrect;
        ::DrawBitmap(m_padrect.x, m_padrect.y, &controlpad);

        // write game status
        string status = "level set: " + m_levelset + "\n";
        if (!m_game.empty())
        {
            status +=
                (m_game().isedit() ? "edit: " : "game: ") +
                m_level + "\n" + m_game().status();
        };
        ifont *font = ::OpenFont(DEFAULTFONT, 16, 1);
        ::SetFont(font, BLACK);
        ::DrawTextRect(
            fld[STAT].x + fld[STAT].w / 16, fld[STAT].y,
            fld[STAT].w - fld[STAT].w / 16, fld[STAT].h,
            (char *) status.c_str(),
            ALIGN_LEFT | VALIGN_MIDDLE);
        ::CloseFont(font);

        // update all bar content
        ::PartialUpdateBW(fld[ALL].x, fld[ALL].y, fld[ALL].w, fld[ALL].h);
    }

    // output one drawpack at once
    void drawone(const drawpack_type &pack)
    {
        DPRINTF("BEGIN %d", pack.size());
        if (!pack.empty())
        {
            // in any drawpack enumerate all cells
            drawpack_type::const_iterator it;
            int mini = m_viewport.height(), maxi = 0;
            int minj = m_viewport.width(), maxj = 0;
            for (it = pack.begin(); it != pack.end(); it++)
            {
                // draw one cell
                ::DrawBitmap(
                    posx() + it->first.j * m_picstore.width(),
                    posy() + it->first.i * m_picstore.height(),
                    m_picstore.mkpic(it->second));

                // store max and min cell coords
                mini = min(mini, it->first.i);
                maxi = max(maxi, it->first.i);
                minj = min(minj, it->first.j);
                maxj = max(maxj, it->first.j);
            };
            // update area that includes all enumerated cells
            ::PartialUpdateBW(
                posx() + minj * m_picstore.width(),
                posy() + mini * m_picstore.height(),
                (maxj + 1 - minj) * m_picstore.width(),
                (maxi + 1 - mini) * m_picstore.height());
        };
        DPRINTF("END %d", pack.size());
    }

    // output all drawpacks from game queue
    void drawqueue(void)
    {
        drawpack_type pack;
        // get next drawpack
        if (m_game().dequeue(pack))
        {
            // update cells in current view
            drawone(m_viewport.translate(pack));

            // viewport and status can be changed
            drawbar();

            // draw queue tail
            m_drawing = true;
            resumequeue();
        }
        else
        {
            // if game is finished, show main menu
            if (m_game().iswon() || m_game().isover())
                menumain();
            // if level is passed, remember it
            if (m_game().iswon())
                m_passed.insert(m_level);
            m_drawing = false;
        };
    }
    void resumequeue(void)
    {
        // tick after 0.05 sec
        if (m_drawing)
            ::SetWeakTimer("zwtimer", timer_handler, 50);
    }
    void suspendqueue(void)
    {
        // unset currently set timer
        ::ClearTimer(timer_handler);
    }

    // translate device keys to application keys
    bool translate(int type, int par1, int par2, evt_type &evt)
    {
        DPRINTF("BEGIN type=%d par1=%d par2=%d", type, par1, par2);
        bool touch = ::QueryTouchpanel() != 0;

        evt = NO_EVT;
        switch (type)
        {
        // short presses
        case EVT_KEYRELEASE:
            if (par2 == 0)
                switch (par1)
                {
                // game keys (360, 301)
                case KEY_UP:
                    evt = UP;
                    break;
                case KEY_DOWN:
                    evt = DOWN;
                    break;
                case KEY_LEFT:
                    evt = LEFT;
                    break;
                case KEY_RIGHT:
                    evt = RIGHT;
                    break;
                case KEY_OK:
                    evt = SWITCH;
                    break;

                // shift menu (301) or main menu (302) hotkey
                case KEY_BACK:
                    evt = MENU;
                    break;

                // main menu hotkey (301)
                case KEY_DELETE:
                    evt = MENU3X3;
                    break;

                // shift menu (360) or right (302) hotkey
                case KEY_PREV:
                    evt = !touch ? MENU3X3 : LEFT;
                    break;
                // main menu (360) or left (302) hotkey
                case KEY_NEXT:
                    evt = !touch ? MENU : RIGHT;
                    break;

                // up and down hotkeys for 302
                case KEY_PREV2:
                    evt = UP;
                    break;
                case KEY_NEXT2:
                    evt = DOWN;
                    break;
                };
            break;

        // long presses
        case EVT_KEYREPEAT:
            if (par2 == 1)
                switch (par1)
                {
                // game key (360, 301)
                case KEY_OK:
                    evt = ACTIVATE;
                    break;

                // shift menu key (360, 301)
                case KEY_UP:
                    evt = MENU3X3;
                    break;

                // main menu key (360, 301)
                case KEY_DOWN:
                    evt = MENU;
                    break;

                // exit hotkey (301)
                case KEY_DELETE:
                    evt = EXIT;
                    break;

                // game hotkey (302)
                case KEY_PREV:
                    evt = !touch ? NO_EVT : ACTIVATE;
                    break;
                // exit (360) or game (302) hotkey
                case KEY_NEXT:
                    evt = !touch ? EXIT : SWITCH;
                    break;

                // shift menu and exit hotkeys for 302
                case KEY_PREV2:
                    evt = MENU3X3;
                    break;
                case KEY_NEXT2:
                    evt = EXIT;
                    break;
                };
            break;

        // simple clicks
        case EVT_POINTERUP:
            // control pad events
            if (par1 >= m_padrect.x && par1 < m_padrect.x + m_padrect.w &&
                par2 >= m_padrect.y && par2 < m_padrect.y + m_padrect.h)
            {
                // button size
                int bw = m_padrect.w / 4;
                int bh = m_padrect.h / 3;
                // clicked button coordinates
                int j = (par1 - m_padrect.x) / bw;
                int i = (par2 - m_padrect.y) / bh;
                int coord = ((i & 0xF) << 4) | (j & 0xF);
                // choose action by button coordinates
                switch (coord)
                {
                case 0x01:
                    evt = UP;
                    break;
                case 0x21:
                    evt = DOWN;
                    break;
                case 0x10:
                    evt = LEFT;
                    break;
                case 0x12:
                    evt = RIGHT;
                    break;
                case 0x11:
                    evt = SWITCH;
                    break;
                case 0x13:
                    evt = ACTIVATE;
                    break;
                case 0x03:
                    evt = MENU3X3;
                    break;
                case 0x23:
                    evt = MENU;
                    break;
                };
            }
            else
            // viewport sight click
            if (par1 >= m_sightrect.x && par1 < m_sightrect.x + m_sightrect.w &&
                par2 >= m_sightrect.y && par2 < m_sightrect.y + m_sightrect.h)
            {
                evt = SHIFTCENTER;
            };
            break;

        case EVT_POINTERDOWN:
            // just shift begin
            evt = SHIFTFIRST;
            break;

        case EVT_POINTERHOLD:
            // shift continuation
            evt = SHIFTNEXT;
            break;
        };
        DPRINTF("END type=%d par1=%d par2=%d result=%d", type, par1, par2, evt);
        return evt != NO_EVT;
    }

    // main application handler
    int handlemain(int type, int par1, int par2)
    {
        DPRINTF("BEGIN type=%d par1=%d par2=%d", type, par1, par2);

        if (!ISKEYEVENT(type) && !ISPOINTEREVENT(type))
        {
            switch (type)
            {
            case EVT_INIT:
                {
                    DPRINTF("device model %s", ::GetDeviceModel());
                    DPRINTF("hardware type %s", ::GetHardwareType());
                    DPRINTF("software version %s", ::GetSoftwareVersion());
                    DPRINTF("hardware depth %d", ::GetHardwareDepth());
                    DPRINTF("serial number %s", ::GetSerialNumber());
                    DPRINTF("waveform filename %s", ::GetWaveformFilename());
                    DPRINTF("device key %s", ::GetDeviceKey());
                    DPRINTF("touch panel %d", ::QueryTouchpanel());

                    // load main config and stored levelset
                    m_levelset = m_serializer.load_config()["levelset"];
                    // or start root levelset
                    if (m_levelset.empty())
                        m_levelset = m_serializer.levelsets().front();
                    // restore game status
                    restore();
                };
                break;

            case EVT_EXIT:
                {
                    // store levelset config
                    store();
                    // store main config
                    stringmap_type cfg;
                    cfg["levelset"] = m_levelset;
                    m_serializer.save_config(cfg);
                };
                break;

            case EVT_ORIENTATION:
                // event from g-sensor
                ::SetOrientation(par1);
                fullredraw();
                break;
            };
        }
        else
        {
            // control event
            evt_type evt;
            if (translate(type, par1, par2, evt))
                switch (evt)
                {
                case LEFT:
                case RIGHT:
                case UP:
                case DOWN:
                case SWITCH:
                case ACTIVATE:
                    // if game exists
                    if (!m_game.empty())
                    {
                        // give it to game
                        m_game().process(game_type::key_type(evt));
                        drawqueue();
                    };
                    break;

                case MENU3X3:
                    // show shift menu for current map
                    if (!m_game.empty())
                        menushift();
                    break;

                case MENU:
                    // show main menu
                    menumain();
                    break;

                case EXIT:
                    // quick exit
                    handlemenu(MENU_EXIT);
                    break;

                case SHIFTFIRST:
                    m_hold = coord_type(par2, par1);
                    break;
                case SHIFTNEXT:
                    {
                        double sx = 1.0 * (par1 - m_hold.j) / (m_viewport.width() * m_picstore.width());
                        double sy = 1.0 * (par2 - m_hold.i) / (m_viewport.height() * m_picstore.height());
                        // shift viewport relative
                        if (m_viewport.shift_rel(sx, sy))
                        {
                            drawone(m_viewport.content());
                            drawbar();
                            m_hold = coord_type(par2, par1);
                        };
                    };
                    break;

                case SHIFTCENTER:
                    {
                        double cx = 1.0 * (par1 - m_sightrect.x) / m_sightrect.w;
                        double cy = 1.0 * (par2 - m_sightrect.y) / m_sightrect.h;
                        // shift viewport
                        if (m_viewport.shift_abs(cx, cy))
                        {
                            drawone(m_viewport.content());
                            drawbar();
                        };
                    };
                    break;
                default:;
                };
        };

        DPRINTF("END type=%d par1=%d par2=%d", type, par1, par2);
        return 0;
    }

    // handler of main menu
    void handlemenu(int index)
    {
        DPRINTF("BEGIN: %d", index);
        resumequeue();

        // for keyboard handler
        m_lastmenu = index;

        switch (index)
        {
        case MENU_NEXT:
            // find and start next level
            {
                // form set of all level names
                stringlist_type levels = m_serializer.levels(m_levelset);
                nameset_type all(levels.begin(), levels.end());

                // form set of non-passed level names
                nameset_type nonpassed;
                std::set_difference(
                    all.begin(), all.end(),
                    m_passed.begin(), m_passed.end(),
                    std::inserter(nonpassed, nonpassed.end()));

                // if nonpassed level names is found, load first of them
                if (!nonpassed.empty())
                    start(true, *nonpassed.begin());
                else
                {
                    message("there are no more non-passed levels in level set");
                    menumain();
                };
            };
            break;

        case MENU_RESTART:
            // start current level again
            start(true, m_level);
            break;

        case MENU_LOADLEVEL:
            // show level choice for current levelset (mark passed)
            menudynamic(
                m_serializer.levels(m_levelset),
                stringlist_type(m_passed.begin(), m_passed.end()),
                MENU_LEVEL);
            break;

        case MENU_EDITLEVEL:
            // start editing current level
            start(false, m_level);
            break;

        case MENU_LOADSOLUTION:
            // show solution choice for current level (mark none)
            menudynamic(
                m_serializer.solutions(m_levelset, m_level),
                stringlist_type(),
                MENU_SOLUTION);
            break;

        case MENU_SAVELEVEL:
        case MENU_SAVESOLUTION:
            // input name for saving
            suspendqueue();
            ::OpenKeyboard("input name", &m_buffer.front(), m_buffer.size() - 1, 0, keyboard_handler);
            break;

        case MENU_LOADLEVELSET:
            // show levelset choice (mark current levelset)
            menudynamic(
                m_serializer.levelsets(),
                stringlist_type(&m_levelset, &m_levelset + 1),
                MENU_LEVELSET);
            break;

        case MENU_EXIT:
            // mark application to exit
            ::CloseApp();
            break;

        default:
            // load new levelset
            if (index >= MENU_LEVELSET && index < MENU_LEVELSET + ITEMLIMIT)
            {
                // store levelset state
                store();
                // change levelset
                m_levelset = m_dynmenustr[index - MENU_LEVELSET];
                // try to load new levelset state
                restore();
            };

            // start new level
            if (index >= MENU_LEVEL && index < MENU_LEVEL + ITEMLIMIT)
            {
                start(true, m_dynmenustr[index - MENU_LEVEL]);
            };

            // start current level with solution
            if (index >= MENU_SOLUTION && index < MENU_SOLUTION + ITEMLIMIT)
            {
                start(true, m_level, m_dynmenustr[index - MENU_SOLUTION]);
            };
        };

        DPRINTF("END: %d", index);
    }

    // handler of shift menu
    void handlemenu3x3(int pos)
    {
        DPRINTF("BEGIN: %d", pos);
        resumequeue();

        if (pos >= 0)
        {
            // if central position selected
            if (pos == viewport_type::HERE)
            {
                // output rotation menu
                suspendqueue();
                ::OpenRotateBox(rotate_handler);
            }
            else
            {
                // or shift viewport
                if (m_viewport.shift(viewport_type::direction_type(pos)))
                    fullredraw();
            };
        };
        DPRINTF("END: %d", pos);
    }

    // keyboard handler
    void handlekeyboard(const char *str)
    {
        DPRINTF("BEGIN: %s", str);
        resumequeue();

        if (str)
        {
            // if name is correct
            if (serializer_type::iscorrect(str))
            {
                // save level
                if (m_lastmenu == MENU_SAVELEVEL)
                {
                    m_level = str;
                    m_serializer.save_level(m_game().data(), m_levelset, m_level);
                };
                // or save solution
                if (m_lastmenu == MENU_SAVESOLUTION)
                    m_serializer.save_solution(m_game().sequence(), m_levelset, m_level, str);
            }
            else
                message("incorrect name, try again");
        };
        DPRINTF("END: %s", str);
    }

    // rotate choice handler
    void handlerotate(int direction)
    {
        DPRINTF("BEGIN: %d", direction);
        resumequeue();
        if (direction >= 0)
            handlemain(EVT_ORIENTATION, direction, 0);
        DPRINTF("END: %d", direction);
    }

    void handletimer(void)
    {
        drawqueue();
    }

    // this static is here because API doesn't support additional
    // void *param in corresponding fuctions ::OpenXXX()
    // BAD PRACTICE
    static
    application_type *s_this;

    static
    int main_handler(int type, int par1, int par2)
    {
        return s_this->handlemain(type, par1, par2);
    }

    static
    void menu_handler(int index)
    {
        s_this->handlemenu(index);
    }

    static
    void menu3x3_handler(int pos)
    {
        s_this->handlemenu3x3(pos);
    }

    static
    void keyboard_handler(char *str)
    {
        s_this->handlekeyboard(str);
    }

    static
    void rotate_handler(int direction)
    {
        s_this->handlerotate(direction);
    }

    static
    void timer_handler(void)
    {
        s_this->handletimer();
    }

public:
    application_type(void):
        m_viewport(1, 1, 1, 1),
        m_buffer(ITEMLIMIT)
    {
    }
    void main(void)
    {
        DPRINTF("BEGIN %d", 0);
        s_this = this;
        ::InkViewMain(main_handler);
        DPRINTF("END %d", 0);
    }
};

application_type *application_type::s_this = NULL;


#endif // _APPLICATION_HXX_
