#ifndef _SERIALIZER_HXX_
#define _SERIALIZER_HXX_


#define SEP                 "/"
#define GAME_DIR            SEP "zwadrax"
#define SUFFIX_LEVEL        ".zwl"
#define SUFFIX_SOLUTION     ".zws"
#define ROOT_LEVELSET       "<root>"
#define CONFIG_MAIN         "main.conf"
#define CONFIG_LEVELSET     "levelset.conf"

#define MAPNAME_AREAWIDTH    "area-width"
#define MAPNAME_AREAHEIGHT   "area-height"
#define MAPNAME_LINE         "line-"
#define MAPNAME_SWITCH       "switch-"

typedef vector<string> stringlist_type;
typedef map<string, string> stringmap_type;

class serializer_type
{
private:
    map<char, game_type::quad_type> m_sym2quad;
    map<game_type::quad_type, char> m_quad2sym;
    map<string, game_type::key_type> m_str2key;
    map<game_type::key_type, string> m_key2str;

    // game working directory
    string m_directory;

    static
    bool isdir(const string &filename)
    {
        struct stat st = {0};
        return
            ::stat(filename.c_str(), &st) == 0 &&
            (st.st_mode & S_IFMT) == S_IFDIR;
    }
    static
    void erase(const string &filename)
    {
        ::unlink(filename.c_str());
    }

    static
    stringlist_type dir2list(const string &dirname)
    {
        stringlist_type strlist;
        DIR *dir = ::opendir(dirname.c_str());
        if (dir != 0)
        {
            struct dirent *ep;
            while ((ep = ::readdir(dir)) != NULL)
                strlist.push_back(ep->d_name);
            ::closedir(dir);
        };
        sort(strlist.begin(), strlist.end());
        return strlist;
    }

    static
    stringlist_type file2list(const string &filename)
    {
        stringlist_type strlist;
        ifstream ifile(filename.c_str());
        string str;
        while (getline(ifile, str))
        {
            if (!str.empty() && str[str.size() - 1] == '\r')
                str.resize(str.size() - 1);
            strlist.push_back(str);
        };
        return strlist;
    }
    static
    void list2file(const stringlist_type &strlist, const string &filename)
    {
        ofstream ofile(filename.c_str());
        stringlist_type::const_iterator it;
        for (it = strlist.begin(); it != strlist.end(); it++)
            ofile << *it << endl;
    }

    static
    stringmap_type file2map(const string &filename)
    {
        stringmap_type strmap;
        stringlist_type strlist = file2list(filename);
        stringlist_type::const_iterator it;
        for (it = strlist.begin(); it != strlist.end(); it++)
        {
            int pos = it->find('=');
            if (pos != int(string::npos))
            {
                string param = it->substr(0, pos);
                string value = it->substr(pos + 1);
                strmap[param] = value;
            };
        };
        return strmap;
    }
    static
    void map2file(const stringmap_type &strmap, const string &filename)
    {
        stringlist_type strlist;
        stringmap_type::const_iterator it;
        for (it = strmap.begin(); it != strmap.end(); it++)
            strlist.push_back(it->first + '=' + it->second);
        list2file(strlist, filename);
    }

    static
    int str2int(const string &str)
    {
        int val = 0;
        istringstream(str) >> val;
        return val;
    }
    static
    string int2str(int val)
    {
        ostringstream ostr;
        ostr << setfill('0') << setw(2) << val;
        return ostr.str();
    }

    static
    string coord2str(const coord_type &coord)
    {
        return int2str(coord.i) + '-' + int2str(coord.j);
    }
    static
    coord_type str2coord(const string &str)
    {
        coord_type coord;
        int pos = str.find('-');
        if (pos != int(string::npos))
        {
            coord.i = str2int(str.substr(0, pos));
            coord.j = str2int(str.substr(pos + 1));
        };
        return coord;
    }

    // dirname for levelset
    string mkname(const string &levelset) const
    {
        return (levelset.empty() || levelset == ROOT_LEVELSET) ?
            m_directory :
            (m_directory + SEP + levelset);
    }
    // filename for level in levelset
    string mkname(const string &levelset, const string &level) const
    {
        return mkname(levelset) + SEP + level + SUFFIX_LEVEL;
    }
    // filename for level solution in levelset
    string mkname(const string &levelset, const string &level, const string &solution) const
    {
        return mkname(levelset) + SEP + level + "." + solution + SUFFIX_SOLUTION;
    }


public:
    serializer_type(void)
    {
        // fill quad-symbol map
        struct { game_type::quad_type quad; char sym; } quads[] = {
            { game_type::QEXIT, 'x' },
            { game_type::QWALL, '#' },
            { game_type::QSTAIRS, '|' },
            { game_type::QSTONE, '*' },
            { game_type::QSTONEANNI, '0' },
            { game_type::QVALVELEFT, '<' },
            { game_type::QVALVERIGHT, '>' },
            { game_type::QVALVEUP, '^' },
            { game_type::QVALVEDOWN, '_' },
            { game_type::QCHARALICE, 'a' },
            { game_type::QCHARBOB, 'b' },
            { game_type::QBRIDGE, '=' },
            { game_type::QBRIDGEBROKEN, '"' },
            { game_type::QTELEPORT, '!' },
            { game_type::QELEVATOR, '$' },
            { game_type::QSWITCHLEFT, '\\' },
            { game_type::QSWITCHRIGHT, '/' }
        };
        for (int i = 0; i < int(sizeof(quads) / sizeof(quads[0])); i++)
        {
            m_sym2quad[quads[i].sym] = quads[i].quad;
            m_quad2sym[quads[i].quad] = quads[i].sym;
        };

        // fill key maps
        struct { game_type::key_type key; const char *str; } keys[] = {
            { game_type::SWITCH, "switch" },
            { game_type::LEFT, "left" },
            { game_type::RIGHT, "right" },
            { game_type::UP, "up" },
            { game_type::DOWN, "down" },
            { game_type::ACTIVATE, "activate" }
        };
        for (int i = 0; i < int(sizeof(keys) / sizeof(keys[0])); i++)
        {
            m_str2key[keys[i].str] = keys[i].key;
            m_key2str[keys[i].key] = keys[i].str;
        };

        // find game data directory
        const char *dirs[] = {
            USERDATA SEP "share" GAME_DIR,
            USERDATA2 SEP "share" GAME_DIR,
            USERDATA GAME_DIR,
            USERDATA2 GAME_DIR,
            FLASHDIR GAME_DIR,
            SDCARDDIR GAME_DIR,
            USBDIR GAME_DIR,
            // for emulator
            "." GAME_DIR
        };
        for (int i = 0; i < int(sizeof(dirs) / sizeof(dirs[0])); i++)
        {
            m_directory = dirs[i];
            if (isdir(dirs[i]) &&
                ::access(dirs[i], R_OK | W_OK | X_OK) == 0)
                break;
        };
    }

    // get all levelsets (will be at least one, first is root)
    stringlist_type levelsets(void) const
    {
        stringlist_type strlist;
        strlist.push_back(ROOT_LEVELSET);
        stringlist_type files = dir2list(m_directory);
        stringlist_type::iterator it;
        for (it = files.begin(); it != files.end(); it++)
        {
            if (isdir(m_directory + SEP + *it) &&
                it->at(0) != '.')
                strlist.push_back(*it);
        };
        return strlist;
    }
    // get all level names in levelset
    stringlist_type levels(const string &levelset) const
    {
        const string suffix = SUFFIX_LEVEL;
        stringlist_type strlist;
        stringlist_type files = dir2list(mkname(levelset));
        stringlist_type::iterator it;
        for (it = files.begin(); it != files.end(); it++)
        {
            if (!isdir(mkname(levelset) + SEP + *it) &&
                it->length() > suffix.length() &&
                it->substr(it->length() - suffix.length()) == suffix)
                strlist.push_back(it->substr(0, it->length() - suffix.length()));
        };
        return strlist;
    }
    // get all solution names for level
    stringlist_type solutions(const string &levelset, const string &level) const
    {
        const string prefix = level + ".", suffix = SUFFIX_SOLUTION;
        stringlist_type strlist;
        stringlist_type files = dir2list(mkname(levelset));
        stringlist_type::iterator it;
        for (it = files.begin(); it != files.end(); it++)
        {
            if (!isdir(mkname(levelset) + SEP + *it) &&
                it->length() > prefix.length() + suffix.length() &&
                it->substr(0, prefix.length()) == prefix &&
                it->substr(it->length() - suffix.length()) == suffix)
                strlist.push_back(
                    it->substr(prefix.length(),
                    it->length() - prefix.length() - suffix.length()));
        };
        return strlist;
    }

    // load main config or levelset config
    stringmap_type load_config(void) const
    {
        return file2map(m_directory + SEP + CONFIG_MAIN);
    }
    stringmap_type load_config(const string &levelset) const
    {
        return file2map(mkname(levelset) + SEP + CONFIG_LEVELSET);
    }
    // save main config or levelset config
    void save_config(const stringmap_type &stringmap) const
    {
        map2file(stringmap, m_directory + SEP + CONFIG_MAIN);
    }
    void save_config(const stringmap_type &stringmap, const string &levelset) const
    {
        map2file(stringmap, mkname(levelset) + SEP + CONFIG_LEVELSET);
    }
    // erase main config or levelset config
    void erase_config(void) const
    {
        erase(m_directory + SEP + CONFIG_MAIN);
    }
    void erase_config(const string &levelset) const
    {
        erase(mkname(levelset) + SEP + CONFIG_LEVELSET);
    }

    // load level by name
    game_type::gamedata_type load_level(
        const string &levelset, const string &level) const
    {
        DPRINTF("BEGIN %s", level.c_str());
        game_type::gamedata_type data;

        stringmap_type levelmap = file2map(mkname(levelset, level));

        int width = str2int(levelmap[MAPNAME_AREAWIDTH]);
        int height = str2int(levelmap[MAPNAME_AREAHEIGHT]);

        DPRINTF("dims %d %d", width, height);

        data.area = game_type::area_type(width, height);

        game_type::coordset_type switchset;
        for (int i = 0; i < data.area.height(); i++)
        {
            string linestr = levelmap[MAPNAME_LINE + int2str(i)];
            linestr.resize(data.area.width(), ' ');

            DPRINTF("line %d -> %s", i, linestr.c_str());

            for (int j = 0; j < data.area.width(); j++)
            {
                if (m_sym2quad.find(linestr[j]) != m_sym2quad.end())
                    data.area.set(coord_type(i, j), m_sym2quad.find(linestr[j])->second);
                // if there is a switch, remember it
                if (data.area.isset(coord_type(i, j), game_type::QSWITCHLEFT) ||
                    data.area.isset(coord_type(i, j), game_type::QSWITCHRIGHT))
                    switchset.insert(coord_type(i, j));
            };
        };

        game_type::coordset_type::iterator it;
        for (it = switchset.begin(); it != switchset.end(); it++)
        {
            string linkname = MAPNAME_SWITCH + coord2str(*it);
            if (levelmap.find(linkname) != levelmap.end())
            {
                string value = levelmap[linkname];
                DPRINTF("switch %d %d -> %s", it->i, it->j, value.c_str());
                stringlist_type sl = split(value);
                if (!sl.empty())
                    data.switchlink[*it] = make_pair(str2coord(sl.front()), str2coord(sl.back()));
            };
        };

        DPRINTF("END %s", level.c_str());
        return data;
    }
    // save level by name
    void save_level(
        const game_type::gamedata_type &data,
        const string &levelset, const string &level) const
    {
        DPRINTF("BEGIN %s", level.c_str());
        stringmap_type levelmap;

        levelmap[MAPNAME_AREAWIDTH] = int2str(data.area.width());
        levelmap[MAPNAME_AREAHEIGHT] = int2str(data.area.height());

        DPRINTF("dims %d %d", data.area.width(), data.area.height());

        for (int i = 0; i < data.area.height(); i++)
        {
            string linestr(data.area.width(), ' ');

            for (int j = 0; j < data.area.width(); j++)
            {
                // store just last, skip others
                map<game_type::quad_type, char>::const_reverse_iterator it;
                for (it = m_quad2sym.rbegin(); it != m_quad2sym.rend(); it++)
                {
                    if (data.area.isset(coord_type(i, j), it->first))
                    {
                        linestr[j] = it->second;
                        break;
                    };
                };
            };

            DPRINTF("line %d -> %s", i, linestr.c_str());
            levelmap[MAPNAME_LINE + int2str(i)] = linestr;
        };

        game_type::switchlink_type::const_iterator it;
        for (it = data.switchlink.begin(); it != data.switchlink.end(); it++)
        {
            stringlist_type strlist;
            strlist.push_back(coord2str(it->second.first));
            if (it->second.first != it->second.second)
                strlist.push_back(coord2str(it->second.second));

            DPRINTF("switch %d %d -> %s", it->first.i, it->first.j, join(strlist).c_str());
            levelmap[MAPNAME_SWITCH + coord2str(it->first)] = join(strlist);
        };

        map2file(levelmap, mkname(levelset, level));
        DPRINTF("END %s", level.c_str());
    }
    // erase level
    void erase_level(const string &levelset, const string &level) const
    {
        erase(mkname(levelset, level));
    }

    // load solution for level
    game_type::sequence_type load_solution(
        const string &levelset, const string &level, const string &solution) const
    {
        game_type::sequence_type sequence;
        stringlist_type strlist = file2list(mkname(levelset, level, solution));
        stringlist_type::const_iterator it;
        for (it = strlist.begin(); it != strlist.end(); it++)
        {
            if (m_str2key.find(*it) != m_str2key.end())
                sequence.push_back(m_str2key.find(*it)->second);
        };
        return sequence;
    }
    // save solution for level
    void save_solution(
        const game_type::sequence_type &sequence,
        const string &levelset, const string &level, const string &solution) const
    {
        stringlist_type strlist;
        game_type::sequence_type::const_iterator it;
        for (it = sequence.begin(); it != sequence.end(); it++)
        {
            if (m_key2str.find(*it) != m_key2str.end())
                strlist.push_back(m_key2str.find(*it)->second);
        };
        list2file(strlist, mkname(levelset, level, solution));
    }
    // erase solution
    void erase_solution(const string &levelset, const string &level, const string &solution) const
    {
        erase(mkname(levelset, level, solution));
    }

    // check, if name for level or solution is correct
    static
    bool iscorrect(const string &name)
    {
        return !name.empty() &&
            name.find('\\') == string::npos &&
            name.find('/') == string::npos &&
            name.find('|') == string::npos;
    }

    // split string to substrings by separator |
    static
    stringlist_type split(const string &src)
    {
        stringlist_type strlist;
        if (!src.empty())
        {
            string str(src);
            int pos;
            while ((pos = str.find('|')) != int(string::npos))
            {
                strlist.push_back(str.substr(0, pos));
                str = str.substr(pos + 1);
            };
            strlist.push_back(str);
        };
        return strlist;
    }
    // join substrings to string with separator |
    static
    string join(const stringlist_type &strlist)
    {
        string str;
        stringlist_type::const_iterator it;
        for (it = strlist.begin(); it != strlist.end(); it++)
            str += ((it == strlist.begin()) ? "" : "|") + *it;
        return str;
    }
};


#endif // _SERIALIZER_HXX_
