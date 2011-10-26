#ifndef IRDA_WRAP_H
#define IRDA_WRAP_H

#if defined(HAVE_IRDA_WINDOWS)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 1
#endif

#include <af_irda.h>
#define irda_device_info _WINDOWS_IRDA_DEVICE_INFO
#define irda_device_list _WINDOWS_DEVICELIST

#define sockaddr_irda _SOCKADDR_IRDA
#define sir_family irdaAddressFamily
#define sir_name   irdaServiceName

#elif defined(HAVE_IRDA_LINUX)

#include "irda.h"

#endif

#endif /* IRDA_WRAP_H */
