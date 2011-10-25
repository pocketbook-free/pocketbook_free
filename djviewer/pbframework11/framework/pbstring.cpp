#include "pbstring.h"

PBString::PBString()
	:std::string()
{
}

PBString::PBString(const char *str)
	:std::string(str)
{
}

PBString::PBString(const std::string & str)
	:std::string(str)
{
}

PBString::~PBString()
{
}

void PBString::Format(const char *format, ...)
{
	char buf[1024];

	va_list Arguments;
	va_start(Arguments, format);
	vsnprintf(buf, sizeof(buf), format, Arguments);
	va_end(Arguments);

	*this = buf;
}

PBString::operator const char *() const
{
	return c_str();
}

bool PBString::TrimLeft()
{
	if (!empty())
	{
		int offset = 0;
		while ( at(offset) == ' ' || at(offset) == '\t' || at(offset) == '\n' || at(offset) == '\r')
			offset++;

		if (offset)
			*this = this->substr(offset);
	}
	return true;
}

bool PBString::TrimRight()
{
	return true;
}

bool PBString::Trim()
{
	return true;
}
