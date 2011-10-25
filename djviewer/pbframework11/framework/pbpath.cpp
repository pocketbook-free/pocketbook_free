#include "pbpath.h"

PBPath::PBPath()
	:PBString()
{
}

PBPath::PBPath(const char *path)
	:PBString(path)
{
}

PBPath::~PBPath()
{
}

const char* PBPath::GetFileName() const
{
	unsigned int pos = find_last_of('/');
	if (pos != STRNULL && pos < (size() - 1) )
		return c_str() + pos + 1;

	return NULL;
}

const char* PBPath::GetMountPrefix() const
{
	if (compare(0, strlen("/mnt/"), "/mnt/") == 0)
	{
		unsigned int pos = find_first_of('/', strlen("/mnt/"));
		if (pos != STRNULL)
		{
			PBString res = substr(0, pos);
			return res;
		}
	}

	return NULL;
}

bool PBPath::OnFlash() const
{
	return (compare(0, strlen(FLASHDIR), FLASHDIR) == 0);
	//return (this->find(FLASHDIR) != STRNULL);
}

bool PBPath::OnSDCard() const
{
	return (compare(0, strlen(SDCARDDIR), SDCARDDIR) == 0);
	//return (this->find(SDCARDDIR) != STRNULL);
}

bool PBPath::OnUSB() const
{
	return (compare(0, strlen(USBDIR), USBDIR) == 0);
	//return (this->find(USBDIR) != STRNULL);
}

const iv_filetype *PBPath::FileType() const
{
	return ::FileType(this->c_str());
}

bool PBPath::IsExist() const
{
	struct stat st;
	return (iv_stat(this->c_str(), &st) != -1);
}

bool PBPath::IsDir() const
{
	struct stat st;
	return (iv_stat(this->c_str(), &st) != -1 && S_ISDIR(st.st_mode));
}

bool PBPath::IsExist(const char *path)
{
	struct stat st;
	return (iv_stat(path, &st) != -1);
}

bool PBPath::IsDir(const char *path)
{
	struct stat st;
	return (iv_stat(path, &st) != -1 && S_ISDIR(st.st_mode));
}
