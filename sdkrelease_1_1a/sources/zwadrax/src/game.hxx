#ifndef _GAME_HXX_
#define _GAME_HXX_


// coordinates
struct coord_type
{
    int i, j;
    coord_type(void): i(0), j(0) {}
    coord_type(int ci, int cj): i(ci), j(cj) {}
    coord_type l(void) const
    {
        return coord_type(i, j - 1);
    }
    coord_type r(void) const
    {
        return coord_type(i, j + 1);
    }
    coord_type u(void) const
    {
        return coord_type(i - 1, j);
    }
    coord_type d(void) const
    {
        return coord_type(i + 1, j);
    }

    friend
    bool operator < (const coord_type &c1, const coord_type &c2)
    {
        return (c1.i != c2.i) ? (c1.i < c2.i) : (c1.j < c2.j);
    }
    friend
    bool operator == (const coord_type &c1, const coord_type &c2)
    {
        return (c1.i == c2.i) && (c1.j == c2.j);
    }
    friend
    bool operator != (const coord_type &c1, const coord_type &c2)
    {
        return !(c1 == c2);
    }
};

// redraw pack - set of pairs (coord, content)
typedef vector<pair<coord_type, int> > drawpack_type;

class game_type
{
public:

    // game manipulation keys
    enum key_type {
        NO_KEY,
        SWITCH,
        LEFT,
        RIGHT,
        UP,
        DOWN,
        ACTIVATE
    };

    // cell content types
    enum quad_type {
        QBACK,
        QEXIT,
        QWALL,
        QSTAIRS,
        QSTONE,
        QSTONEANNI,
        QVALVELEFT,
        QVALVERIGHT,
        QVALVEUP,
        QVALVEDOWN,
        QCHARALICE,
        QCHARBOB,
        QCHARDEAD,
        QBRIDGE,
        QBRIDGEBROKEN,
        QTELEPORT,
        QELEVATOR,
        QSWITCHLEFT,
        QSWITCHRIGHT,
        QFLASH,
        QSELECTOR
    };

    // game area
    class area_type
    {
    private:
        int m_width;
        vector<int> m_data;

    public:
        area_type(): m_width(0) {}
        area_type(int width, int height): m_width(width)
        {
            assert(width > 0 && height > 0);
            m_data.assign(width * height, 0);
        }

        int width(void) const
        {
            return m_width;
        }
        int height(void) const
        {
            return (m_width == 0) ? 0 : m_data.size() / m_width;
        }

        int &cell(const coord_type &coord)
        {
            assert(coord.i >=0 && coord.i < height());
            assert(coord.j >=0 && coord.j < width());
            return m_data[width() * coord.i + coord.j];
        }
        int cell(const coord_type &coord) const
        {
            return const_cast<area_type *>(this)->cell(coord);
        }

        void set(const coord_type &coord, quad_type type)
        {
            assert(type >= 0);
            cell(coord) |= (1 << type);
        }
        void reset(const coord_type &coord, quad_type type)
        {
            assert(type >= 0);
            cell(coord) &= ~(1 << type);
        }
        bool isset(const coord_type &coord, quad_type type) const
        {
            assert(type >= 0);
            return (cell(coord) & (1 << type)) != 0;
        }
    };
    // switch links
    typedef map<coord_type, pair<coord_type, coord_type> > switchlink_type;
    // input keys sequence
    typedef vector<key_type> sequence_type;
    // set of cell coordinates
    typedef set<coord_type> coordset_type;
    // list of quads
    typedef vector<quad_type> quadlist_type;

    struct gamedata_type
    {
        area_type area;
        switchlink_type switchlink;
    };

private:

    enum state_type {
        STATEGAME,
        STATEOVER,
        STATEWIN,
        STATEEDIT
    };

    enum {
        HEIGHTMAX = 2
    };

    // redrawing queue
    list<drawpack_type> m_drawqueue;
    // game area
    area_type m_area;
    // links of switches
    switchlink_type m_switchlink;
    // processed keys sequence
    sequence_type m_sequence;

    // game state
    state_type m_state;
    // character and selector coordinates
    coord_type m_coordalice, m_coordbob, m_coordselector;
    // current active character
    quad_type m_currentchar;
    // flag if active character is wants up
    bool m_wantsup;
    // linking state for edit mode
    vector<coord_type> m_linkstate;


    // ============================================================
    // redrawing queue enqueue routines (redraw all args at once)
    // ============================================================
    void enqueue(const coordset_type &coords)
    {
        if (!coords.empty())
        {
            drawpack_type pack;
            coordset_type::const_iterator it;
            for (it = coords.begin(); it != coords.end(); it++)
            {
                if (it->i >= 0 && it->i < m_area.height() &&
                    it->j >= 0 && it->j < m_area.width())
                {
                    m_area.set(*it, QBACK);
                    pack.push_back(make_pair(*it, m_area.cell(*it)));
                    m_area.reset(*it, QBACK);
                };
            };
            m_drawqueue.push_back(pack);
        };
    }
    void enqueue(const coord_type &coord)
    {
        coordset_type coordset;
        coordset.insert(coord);
        enqueue(coordset);
    }
    void enqueue(const coord_type &coord1, const coord_type &coord2)
    {
        coordset_type coordset;
        coordset.insert(coord1);
        coordset.insert(coord2);
        enqueue(coordset);
    }
    void enqueue(const coord_type &coord1, const coord_type &coord2, const coord_type &coord3)
    {
        coordset_type coordset;
        coordset.insert(coord1);
        coordset.insert(coord2);
        coordset.insert(coord3);
        enqueue(coordset);
    }

    // ============================================================
    // cell manipulation routines
    // ============================================================

    // check, if there quad is present at coord
    bool isthere(const coord_type &coord, quad_type quad) const
    {
        bool rc;
        if (coord.i < 0 || coord.i >= m_area.height() ||
            coord.j < 0 || coord.j >= m_area.width())
            rc = (quad == QWALL);
        else
            rc = (quad == QBACK || m_area.isset(coord, quad));
        return rc;
    }

    // check, if there is no free space at coord
    bool isoccupied(const coord_type &coord) const
    {
        bool rc = false;
        const quad_type occupier[] =
        {
            QWALL,
            QSTONE,
            QSTONEANNI,
            QVALVELEFT,
            QVALVERIGHT,
            QVALVEUP,
            QVALVEDOWN,
            QBRIDGE,
            QELEVATOR
        };
        for (int i = 0; !rc && i < int(sizeof(occupier) / sizeof(occupier[0])); i++)
            rc = isthere(coord, occupier[i]);
        return rc;
    }

    // check, if there movable quad is present at coord
    bool ismovable(const coord_type &coord, quad_type &quad) const
    {
        bool rc = false;
        const quad_type movable[] =
        {
            QSTONE,
            QSTONEANNI
        };
        for (int i = 0; !rc && i < int(sizeof(movable) / sizeof(movable[0])); i++)
        {
            rc = isthere(coord, movable[i]);
            if (rc)
                quad = movable[i];
        };
        return rc;
    }

    // check, if there one or more characters present at coord
    bool ischaracter(const coord_type &coord, quadlist_type &ch) const
    {
        const quad_type character[] =
        {
            QCHARALICE,
            QCHARBOB,
            QCHARDEAD
        };
        ch.clear();
        for (int i = 0; i < int(sizeof(character) / sizeof(character[0])); i++)
            if (isthere(coord, character[i]))
                ch.push_back(character[i]);
        return !ch.empty();
    }

    // locate both characters
    void locate(void)
    {
        for (int i = 0; i < m_area.height(); i++)
        {
            for (int j = 0; j < m_area.width(); j++)
            {
                // remember coordinates of both characters
                if (isthere(coord_type(i, j), QCHARALICE))
                    m_coordalice = coord_type(i, j);
                if (isthere(coord_type(i, j), QCHARBOB))
                    m_coordbob = coord_type(i, j);
            };
        };
    }

    // return coord of active character
    coord_type & active(void)
    {
        if (m_state == STATEEDIT)
            return m_coordselector;
        else
            return (m_currentchar == QCHARALICE) ? m_coordalice : m_coordbob;
    }

    // replace one quad by another at coord
    void replace(const coord_type &coord, quad_type quadfrom, quad_type quadto)
    {
        if (isthere(coord, quadfrom))
        {
            m_area.reset(coord, quadfrom);
            m_area.set(coord, quadto);
        };
    }

    // move quad from one coord to another
    void move(const coord_type &coordfrom, const coord_type &coordto, quad_type quad)
    {
        m_area.reset(coordfrom, quad);
        m_area.set(coordto, quad);

        if (quad == QCHARALICE)
            m_coordalice = coordto;
        if (quad == QCHARBOB)
            m_coordbob = coordto;
        if (quad == QSELECTOR)
            m_coordselector = coordto;
    }

    // move all movable quads from one coord to another
    bool moveall(const coord_type &coordfrom, const coord_type &coordto)
    {
        // there only characters and stones can fall or teleport
        quadlist_type fallable;
        ischaracter(coordfrom, fallable);
        quad_type quad;
        if (ismovable(coordfrom, quad))
            fallable.push_back(quad);

        // move all fallables
        quadlist_type::iterator it;
        for (it = fallable.begin(); it != fallable.end(); it++)
            move(coordfrom, coordto, *it);
        return !fallable.empty();
    }

    // add quad to coord
    void add(const coord_type &coord, quad_type quad)
    {
        m_area.set(coord, quad);
    }

    // remove quad from coord
    void remove(const coord_type &coord, quad_type quad)
    {
        m_area.reset(coord, quad);
    }

    // generate full coordset for area
    coordset_type fullset(void) const
    {
        coordset_type allcoords;
        for (int i = 0; i < m_area.height(); i++)
            for (int j = 0; j < m_area.width(); j++)
                allcoords.insert(coord_type(i, j));
        return allcoords;
    }


    // ============================================================
    // game routines
    // ============================================================

    coordset_type toggle(const coord_type &coord)
    {
        DPRINTF("BEGIN i=%d j=%d", coord.i, coord.j);

        coordset_type changed;

        if (m_switchlink.find(coord) != m_switchlink.end())
        {
            // coord of acting quad
            coord_type actcoord = m_switchlink[coord].first;

            struct { quad_type from; quad_type to; } tr[] = {
                { QVALVELEFT, QVALVERIGHT },
                { QVALVERIGHT, QVALVELEFT },
                { QVALVEUP, QVALVEDOWN },
                { QVALVEDOWN, QVALVEUP },
                { QBRIDGE, QBRIDGEBROKEN },
                { QBRIDGEBROKEN, QBRIDGE }
            };

            // search for selfacting quad in cell
            for (int i = 0; i < int(sizeof(tr) / sizeof(tr[0])); i++)
            {
                // if found, toggle it
                if (isthere(actcoord, tr[i].from))
                {
                    // toggle quad if it isn't a broken bridge
                    // or if place under broken bridge isn't occupied
                    if (!isthere(actcoord, QBRIDGEBROKEN) ||
                        !isoccupied(actcoord))
                    {
                        replace(actcoord, tr[i].from, tr[i].to);
                        enqueue(actcoord);
                        changed.insert(actcoord);
                        break;
                    };
                };
            };

            // if there is a teleport
            if (isthere(actcoord, QTELEPORT))
            {
                // get coord of destination
                coord_type dstcoord = m_switchlink[coord].second;
                // if destination is free
                if (!isoccupied(dstcoord))
                {
                    // flash source
                    add(actcoord, QFLASH);
                    enqueue(actcoord);

                    // move content and flash from source to destination
                    remove(actcoord, QFLASH);
                    moveall(actcoord, dstcoord);
                    add(dstcoord, QFLASH);

                    enqueue(actcoord);
                    enqueue(dstcoord);

                    // unflash destination
                    remove(dstcoord, QFLASH);
                    enqueue(dstcoord);

                    changed.insert(actcoord);
                    changed.insert(dstcoord);
                };
            };

            // if there is an elevator
            if (isthere(actcoord, QELEVATOR))
            {
                // get destination coord
                coord_type dstcoord = m_switchlink[coord].second;
                // destination must be at the same column
                dstcoord.j = actcoord.j;

                // are we moving down?
                bool down = actcoord < dstcoord;
                // are we moving back (undo)
                bool back = false;

                coord_type cur = actcoord;
                // while we aren't came to destination or back to source
                while ((!back && cur != dstcoord) || (back && cur != actcoord))
                {
                    quad_type quad;
                    // if we are moving down and lower cell is free
                    // or if we are moving up and upper cell is free
                    // or if we are moving up and dragging stone and upper-upper cell is free
                    if ((down && !isoccupied(cur.d())) ||
                        (!down && !isoccupied(cur.u())) ||
                        (!down && ismovable(cur.u(), quad) && !isoccupied(cur.u().u())))
                    {
                        // calc next coord
                        coord_type next = down ? cur.d() : cur.u();

                        // move elevator
                        move(cur, next, QELEVATOR);
                        // move content
                        moveall(cur.u(), next.u());

                        // generate coordset of three changed cells
                        coordset_type coordset;
                        coordset.insert(cur);
                        coordset.insert(cur.u());
                        coordset.insert(next);
                        coordset.insert(next.u());
                        // redraw all
                        enqueue(coordset);
                        // balance map
                        balance(coordset);

                        // change current elevator position
                        cur = next;
                    }
                    else
                    {
                        // if next moving is impossible, move back (undo)
                        down = !down;
                        if (!back)
                            back = true;
                        else
                            // if we are already moving back, leave cycle
                            break;
                    };
                };

                // if elevator has been moved
                if (cur != actcoord)
                {
                    // store new position
                    m_switchlink[coord] = make_pair(cur, actcoord);
                    // mark toggling as successful
                    changed.insert(dstcoord);
                };
            };

            // redraw active character (for viewport shifting back)
            enqueue(active());
        };

        DPRINTF("END i=%d j=%d changed=%d", coord.i, coord.j, !changed.empty());
        return changed;
    }

    void balance(const coordset_type ch)
    {
        // store locations of characters
        coord_type coordalice = m_coordalice;
        coord_type coordbob = m_coordbob;

        // set of last changed cells
        coordset_type changed(ch);

        while (!changed.empty())
        {
            // ===== check for stone death =====
            // if there is stone falling on character
            // or moved over character, kill him
            {
                quadlist_type character;
                coordset_type::const_iterator it;
                for (it = changed.begin(); it != changed.end(); it++)
                {
                    if (ischaracter(*it, character) && isoccupied(*it))
                    {
                        replace(*it, QCHARALICE, QCHARDEAD);
                        replace(*it, QCHARBOB, QCHARDEAD);
                        enqueue(*it);
                        m_state = STATEOVER;
                    };
                };
            };

            // ===== neighbours set =====
            // calculate extended set from changed cells and neighbours
            coordset_type neighbor;
            {
                coordset_type::const_iterator it;
                for (it = changed.begin(); it != changed.end(); it++)
                {
                    // the cell itself
                    neighbor.insert(*it);
                    // cell neighbours
                    neighbor.insert(it->r());
                    neighbor.insert(it->l());
                    neighbor.insert(it->u());
                    neighbor.insert(it->d());
                };
            };
            // we don't need this set anymore
            changed.clear();

            // ===== stones annihilation =====
            // search for stones that that touch each other
            {
                coordset_type disappear;
                coordset_type::iterator it;
                for (it = neighbor.begin(); it != neighbor.end(); it++)
                {
                    // if here is annihilation stone
                    if (isthere(*it, QSTONEANNI))
                    {
                        // and it touches other such stone
                        if (isthere(it->r(), QSTONEANNI) ||
                            isthere(it->l(), QSTONEANNI) ||
                            isthere(it->u(), QSTONEANNI) ||
                            isthere(it->d(), QSTONEANNI))
                        {
                            // mark it for disappearing
                            disappear.insert(*it);
                        };
                    };
                };
                // flash all disappearing stones
                for (it = disappear.begin(); it != disappear.end(); it++)
                    add(*it, QFLASH);
                enqueue(disappear);
                // liquidate all stones
                for (it = disappear.begin(); it != disappear.end(); it++)
                {
                    remove(*it, QFLASH);
                    remove(*it, QSTONEANNI);
                };
                enqueue(disappear);
                changed.insert(disappear.begin(), disappear.end());
            };

            // ========= gravitation =========
            {
                coordset_type fall;
                // enumerate in back order (from lower layers to upper)
                coordset_type::reverse_iterator it;
                for (it = neighbor.rbegin(); it != neighbor.rend(); it++)
                {
                    // if there is a place under the cell
                    if (!isoccupied(it->d()))
                    {
                        // if here is no stairs for characters
                        quad_type quad;
                        if (!isthere(it->d(), QSTAIRS) ||
                            ismovable(*it, quad))
                        {
                            // move down all that can be moved
                            if (moveall(*it, it->d()))
                            {
                                // redraw cell
                                fall.insert(*it);
                                fall.insert(it->d());
                            };
                        };
                    };
                };
                enqueue(fall);
                changed.insert(fall.begin(), fall.end());
            };
        };

        // ===== check for falling death =====
        // if character falls from more than two quads, kill him
        if (m_coordalice.i - coordalice.i > HEIGHTMAX)
        {
            replace(m_coordalice, QCHARALICE, QCHARDEAD);
            enqueue(m_coordalice);
            m_state = STATEOVER;
        };
        if (m_coordbob.i - coordbob.i > HEIGHTMAX)
        {
            replace(m_coordbob, QCHARBOB, QCHARDEAD);
            enqueue(m_coordbob);
            m_state = STATEOVER;
        };

        // ===== check for win =====
        // if both characters came to exit, game finished
        if (m_state == STATEGAME)
        {
            if (m_coordalice == m_coordbob &&
                isthere(active(), QEXIT))
            {
                add(active(), QFLASH);
                enqueue(active());
                remove(active(), QFLASH);
                remove(active(), QCHARALICE);
                remove(active(), QCHARBOB);
                enqueue(active());
                m_state = STATEWIN;
            }
        };
    }

    // process user action
    void act(key_type key)
    {
        m_sequence.push_back(key);

        coordset_type changed;

        switch (key)
        {
        case SWITCH:
            // change active character
            m_currentchar =
                (m_currentchar == QCHARALICE) ?
                QCHARBOB : QCHARALICE;

            // flash new character
            add(active(), QFLASH);
            enqueue(active());
            // clear flash
            remove(active(), QFLASH);
            enqueue(active());
            break;

        case ACTIVATE:
            // if here is switch
            if (isthere(active(), QSWITCHLEFT) ||
                isthere(active(), QSWITCHRIGHT))
            {
                // switch in OFF state
                quad_type off =
                    isthere(active(), QSWITCHLEFT) ?
                    QSWITCHLEFT : QSWITCHRIGHT;
                // switch in ON state
                quad_type on =
                    (off == QSWITCHLEFT) ?
                    QSWITCHRIGHT : QSWITCHLEFT;

                // turn switch on
                replace(active(), off, on);
                enqueue(active());

                // try to toggle switch
                coordset_type chg = toggle(active());
                if (!chg.empty())
                    changed.insert(chg.begin(), chg.end());
                else
                {
                    // turn switch back
                    replace(active(), on, off);
                    enqueue(active());
                };
            };
            break;

        case LEFT:
        case RIGHT:
        case UP:
        case DOWN:
            {
                // new potentional coord
                coord_type ncoord = active();
                switch (key)
                {
                case UP:
                    // if there is a valve, new coord is over it
                    if (isthere(active().u(), QVALVEUP))
                        ncoord = active().u().u();
                    // if there is a stairs, new coord is next
                    else if (isthere(active(), QSTAIRS))
                        ncoord = active().u();
                    else
                        // in other cases char just wants up
                        m_wantsup = true;
                    break;
                case DOWN:
                    // if there is a valve, new coord is under it
                    if (isthere(active().d(), QVALVEDOWN))
                        ncoord = active().d().d();
                    // if there is a stairs, new coord is next
                    else if (isthere(active().d(), QSTAIRS))
                        ncoord = active().d();
                    break;
                case LEFT:
                case RIGHT:
                    // new coord is nearest from left or right
                    ncoord = (key == LEFT) ? active().l() : active().r();
                    // if new coord is occupied and char wants up and
                    // coord over char is free, new coord is over itself
                    if (m_wantsup && isoccupied(ncoord) && !isoccupied(active().u()))
                        ncoord = ncoord.u();
                    // if here is corresponding valve, new coord is beyond it
                    if ((key == LEFT && isthere(ncoord, QVALVELEFT)) ||
                        (key == RIGHT && isthere(ncoord, QVALVERIGHT)))
                        ncoord = (key == LEFT) ? ncoord.l() : ncoord.r();
                    break;
                default:;
                };
                // if free movement was found, try to move
                if (ncoord != active() && !isoccupied(ncoord))
                {
                    coord_type ocoord = active();
                    move(ocoord, ncoord, m_currentchar);
                    enqueue(ocoord, ncoord);
                    changed.insert(ocoord);
                    changed.insert(ncoord);
                }
                else if (key == LEFT || key == RIGHT)
                {
                    ncoord = (key == LEFT) ? active().l() : active().r();
                    // if there is a stone on the way
                    quad_type quad;
                    if (ismovable(ncoord, quad))
                    {
                        coord_type nncoord = (key == LEFT) ? ncoord.l() : ncoord.r();
                        // try to move it forward
                        if (!isoccupied(nncoord))
                        {
                            coord_type ocoord = active();
                            move(ncoord, nncoord, quad);
                            move(ocoord, ncoord, m_currentchar);
                            enqueue(ocoord, ncoord, nncoord);
                            changed.insert(ocoord);
                            changed.insert(ncoord);
                            changed.insert(nncoord);
                        };
                    };
                };
            };
            break;
        default:;
        };

        // any key except UP resets wantsup flag
        m_wantsup = (key == UP) ? m_wantsup: false;

        balance(changed);
    }

    // ============================================================
    // edit routines
    // ============================================================

    // process user action
    void plot(key_type key)
    {
        // new coord of selector
        coord_type ncoord = active();
        switch (key)
        {
        case LEFT:
            ncoord = active().l();
            break;
        case RIGHT:
            ncoord = active().r();
            break;
        case UP:
            ncoord = active().u();
            break;
        case DOWN:
            ncoord = active().d();
            break;

        case SWITCH:
            // change current content
            {
                quad_type q[] = {
                    QWALL,
                    QSTAIRS,
                    QSWITCHLEFT,
                    QSWITCHRIGHT,
                    QVALVELEFT,
                    QVALVERIGHT,
                    QVALVEUP,
                    QVALVEDOWN,
                    QSTONE,
                    QSTONEANNI,
                    QBRIDGE,
                    QBRIDGEBROKEN,
                    QTELEPORT,
                    QELEVATOR,
                    QCHARALICE,
                    QCHARBOB,
                    QEXIT
                };

                int src, dst;
                // search for src quad in cell
                for (src = 0; src < int(sizeof(q) / sizeof(q[0])); src++)
                {
                    if (isthere(active(), q[src]))
                        break;
                };
                // if src quad not found, dst is blank, else is next
                dst = (src == int(sizeof(q) / sizeof(q[0]))) ? 0 : src + 1;

                // remove src quad if found
                if (src < int(sizeof(q) / sizeof(q[0])))
                    remove(active(), q[src]);
                // add dst quad if found
                if (dst < int(sizeof(q) / sizeof(q[0])))
                    add(active(), q[dst]);
                // redraw cell
                enqueue(active());
            };
            break;

        case ACTIVATE:
            // create link (three cells)
            if (m_linkstate.size() <= 1)
            {
                // if first or second cell in link,
                // flash it and remember
                add(active(), QFLASH);
                enqueue(active());

                m_linkstate.push_back(active());
            }
            else
            {
                // if is the third cell (last), flash it
                add(active(), QFLASH);
                enqueue(active());
                // remove all flashes
                remove(m_linkstate.front(), QFLASH);
                remove(m_linkstate.back(), QFLASH);
                remove(active(), QFLASH);
                enqueue(m_linkstate.front(), m_linkstate.back(), active());

                // remember full link
                m_switchlink[m_linkstate.front()] = make_pair(m_linkstate.back(), active());
                // clear remembered cells
                m_linkstate.clear();
            };
            break;

        default:;
        };

        // if selector position is have to change
        if (ncoord != active() &&
            ncoord.i >= 0 && ncoord.i < m_area.height() &&
            ncoord.j >= 0 && ncoord.j < m_area.width())
        {
            // move selector
            coord_type ocoord = active();
            move(ocoord, ncoord, QSELECTOR);
            enqueue(ocoord, ncoord);

            // if new position is linked, flash destinations
            if (m_switchlink.find(active()) != m_switchlink.end())
            {
                add(m_switchlink[active()].first, QFLASH);
                add(m_switchlink[active()].second, QFLASH);
                enqueue(m_switchlink[active()].first, m_switchlink[active()].second);

                remove(m_switchlink[active()].first, QFLASH);
                remove(m_switchlink[active()].second, QFLASH);
                enqueue(m_switchlink[active()].first, m_switchlink[active()].second);
            };
        };
    }

public:

    // constructor for game in game mode
    game_type(const gamedata_type &gamedata, const sequence_type &sequence, bool play):
        m_area(gamedata.area),
        m_switchlink(gamedata.switchlink),
        m_state(STATEGAME),
        m_currentchar(QCHARALICE),
        m_wantsup(false)
    {
        // search for characters
        locate();

        // generate full coordset
        coordset_type allcoords = fullset();
        // full refresh
        enqueue(allcoords);
        // first balancing
        balance(allcoords);

        // process all input keys
        sequence_type::const_iterator it;
        for (it = sequence.begin(); it != sequence.end(); it++)
            process(*it);

        // if replay doesn't requested, clean queue and add one pack
        if (!play)
        {
            m_drawqueue.clear();
            enqueue(allcoords);
        };

        // flash active character
        if (m_state == STATEGAME)
        {
            add(active(), QFLASH);
            enqueue(active());
            remove(active(), QFLASH);
            enqueue(active());
        };
    }

    // constructor for game in edit mode
    game_type(const gamedata_type &gamedata):
        m_area(gamedata.area),
        m_switchlink(gamedata.switchlink),
        m_state(STATEEDIT)
    {
        // full refresh
        enqueue(fullset());

        // show selector
        add(active(), QSELECTOR);
        enqueue(active());
    }

    int width(void) const
    {
        return m_area.width();
    }

    int height(void) const
    {
        return m_area.height();
    }

    bool isover(void) const
    {
        return m_state == STATEOVER;
    }

    bool iswon(void) const
    {
        return m_state == STATEWIN;
    }

    bool isedit(void) const
    {
        return m_state == STATEEDIT;
    }

    string status(void) const
    {
        string st;
        switch (m_state)
        {
        case STATEGAME:
            st = string("active: ") + ((m_currentchar == QCHARALICE) ? "Alice" : "Bob");
            break;
        case STATEWIN:
            st = "you win!";
            break;
        case STATEOVER:
            st = "game over.";
            break;
        case STATEEDIT:
            if (m_linkstate.empty())
            {
                switchlink_type::const_iterator it = m_switchlink.find(m_coordselector);
                if (it != m_switchlink.end())
                    st = string("cell is linked to ") +
                        ((it->second.first == it->second.second) ? "cell" : "pair");
            }
            else
                st = string("linking ") + ((m_linkstate.size() == 1) ? "first" : "second");
            break;
        };
        return st;
    }

    void process(key_type key)
    {
        DPRINTF("BEGIN key=%d state=%d", key, m_state);

        if (m_state == STATEGAME)
            act(key);
        if (m_state == STATEEDIT)
            plot(key);

        DPRINTF("END key=%d state=%d", key, m_state);
    }

    bool dequeue(drawpack_type &pack)
    {
        if (m_drawqueue.empty())
            pack.clear();
        else
        {
            pack = m_drawqueue.front();
            m_drawqueue.pop_front();
        };
        return !pack.empty();
    }

    gamedata_type data(void) const
    {
        gamedata_type data = { m_area, m_switchlink };
        return data;
    }

    sequence_type sequence(void) const
    {
        return m_sequence;
    }
};


#endif // _GAME_HXX_
