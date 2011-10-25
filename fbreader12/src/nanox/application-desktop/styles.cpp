#include <inkview.h>
#include <inkinternal.h>
#include <ZLibrary.h>
#include "main.h"
std::string stylesFileName;

void read_styles() {
        stylesFileName = std::string(CONFIGPATH) + "/" + "styles.xml";
        struct stat st;
        if (stat(stylesFileName.c_str(), &st) == -1) {
                stylesFileName = ZLibrary::DefaultFilesPathPrefix() + "styles.xml";
	}
        fprintf(stderr, "using %s\n", stylesFileName.c_str());
}
