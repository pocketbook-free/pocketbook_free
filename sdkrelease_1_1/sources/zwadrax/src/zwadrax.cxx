#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cassert>

#include "inkview.h"

// #define DPRINTF(fmt, ...)

//*
#define DPRINTF(fmt, ...) \
    fprintf( \
        stderr, \
        "%s:%d:%s " fmt "\n", \
        __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
//*/

using namespace std;

#include "game.hxx"
#include "viewport.hxx"
#include "serializer.hxx"
#include "picstore.hxx"
#include "application.hxx"


int main(int argc, char *argv[])
{
    application_type().main();
    return 0;
}

