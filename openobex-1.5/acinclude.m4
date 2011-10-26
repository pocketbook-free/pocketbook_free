AC_DEFUN([AC_PROG_CC_PIE], [
	AC_CACHE_CHECK([whether ${CC-cc} accepts -fPIE], ac_cv_prog_cc_pie, [
		echo 'void f(){}' > conftest.c
		if test -z "`${CC-cc} -fPIE -pie -c conftest.c 2>&1`"; then
			ac_cv_prog_cc_pie=yes
		else
			ac_cv_prog_cc_pie=no
		fi
		rm -rf conftest*
	])
])

AC_DEFUN([AC_INIT_OPENOBEX], [
	AC_PREFIX_DEFAULT(/usr/local)

	if (test "${CFLAGS}" = ""); then
		CFLAGS="-Wall -O2"
	fi

	if (test "${prefix}" = "NONE"); then
		dnl no prefix and no sysconfdir, so default to /etc
		if (test "$sysconfdir" = '${prefix}/etc'); then
			AC_SUBST([sysconfdir], ['/etc'])
		fi

		dnl no prefix and no mandir, so use ${prefix}/share/man as default
		if (test "$mandir" = '${prefix}/man'); then
			AC_SUBST([mandir], ['${prefix}/share/man'])
		fi

		prefix="${ac_default_prefix}"
	fi

	if (test "${libdir}" = '${exec_prefix}/lib'); then
		libdir="${prefix}/lib"
	fi

	if (test "$sysconfdir" = '${prefix}/etc'); then
		configdir="${prefix}/etc/openobex"
	else
		configdir="${sysconfdir}/openobex"
	fi

	AC_DEFINE_UNQUOTED(CONFIGDIR, "${configdir}", [Directory for the configuration files])
])

AC_DEFUN([AC_PATH_WIN32], [
	case $host in
	*-*-mingw32*)
		EXTRA_LIBS="$EXTRA_LIBS -lws2_32"
		;;
	esac
	AC_SUBST(EXTRA_LIBS)
])

AC_DEFUN([AC_PATH_IRDA_LINUX], [
	AC_CACHE_CHECK([for IrDA support], irda_found, [
		AC_TRY_COMPILE([
				#include <sys/socket.h>
				#include "lib/irda.h"
			], [
				struct irda_device_list l;
			], irda_found=yes, irda_found=no)
	])
	irda_linux=$irda_found
])

AC_DEFUN([AC_PATH_IRDA_WIN32], [
	AC_CACHE_VAL(irda_found, [
		AC_CHECK_HEADERS(af_irda.h, irda_found=yes, irda_found=no,
				 [
				  #include <winsock2.h>
		])
	])
      irda_windows=$irda_found
	AC_MSG_CHECKING([for IrDA support])
	AC_MSG_RESULT([$irda_found])
])

AC_DEFUN([AC_PATH_IRDA], [
	case $host in
	*-*-linux*)
		AC_PATH_IRDA_LINUX
		;;
	*-*-mingw32*)
		AC_PATH_IRDA_WIN32
		;;
	*)
		irda_found=no;
		AC_MSG_CHECKING([for IrDA support])
		AC_MSG_RESULT([$irda_found])
		;;
	esac
])

AC_DEFUN([AC_PATH_WINBT], [
	AC_CACHE_VAL(winbt_found,[
               AC_CHECK_HEADERS(ws2bth.h, winbt_found=yes, winbt_found=no,
                                [
                                  #include <winsock2.h>
               ])
	])
	AC_MSG_CHECKING([for Windows Bluetooth support])
	AC_MSG_RESULT([$winbt_found])
])


AC_DEFUN([AC_PATH_NETBSDBT], [
	AC_CACHE_CHECK([for NetBSD Bluetooth support], netbsdbt_found, [
		AC_TRY_COMPILE([
				#include <bluetooth.h>
			], [
				struct sockaddr_bt *bt;
			], netbsdbt_found=yes, netbsdbt_found=no)
	])
])

AC_DEFUN([AC_PATH_FREEBSDBT], [
	AC_CACHE_CHECK([for FreeBSD Bluetooth support], freebsdbt_found, [
		AC_TRY_COMPILE([
				#include <bluetooth.h>
			], [
				struct sockaddr_rfcomm *rfcomm;
			], freebsdbt_found=yes, freebsdbt_found=no)
	])
])

AC_DEFUN([AC_PATH_BLUEZ], [
	PKG_CHECK_MODULES(BLUETOOTH, bluez, bluez_found=yes, AC_MSG_RESULT(no))
])

AC_DEFUN([AC_PATH_BLUETOOTH], [
	case $host in
	*-*-linux*)
		AC_PATH_BLUEZ
		;;
	*-*-freebsd*)
		AC_PATH_FREEBSDBT
		;;
	*-*-netbsd*)
		AC_PATH_NETBSDBT
		;;
	*-*-mingw32*)
		AC_PATH_WINBT
		;;
	esac
	AC_SUBST(BLUETOOTH_CFLAGS)
	AC_SUBST(BLUETOOTH_LIBS)
])

AC_DEFUN([AC_PATH_USB], [
	usb_lib_found=no
	case $host in
	*-*-mingw32*)
		USB_CFLAGS=""
		USB_LIBS="-lusb"
		usb_lib_found=yes
		;;
	*)
		PKG_CHECK_MODULES(USB, libusb, usb_lib_found=yes, AC_MSG_RESULT(no))
		AC_CHECK_FILE(${prefix}/lib/pkgconfig/libusb.pc, REQUIRES="libusb")
		;;
	esac
	AC_SUBST(USB_CFLAGS)
	AC_SUBST(USB_LIBS)

	usb_get_busses=no
	AC_CHECK_LIB(usb, usb_get_busses, usb_get_busses=yes, AC_DEFINE(NEED_USB_GET_BUSSES, 1, [Define to 1 if you need the usb_get_busses() function.]))

	usb_interrupt_read=no
	AC_CHECK_LIB(usb, usb_interrupt_read, usb_interrupt_read=yes, AC_DEFINE(NEED_USB_INTERRUPT_READ, 1, [Define to 1 if you need the usb_interrupt_read() function.]))

	if (test "$usb_lib_found" = "yes" && test "$usb_get_busses" = "yes" && test "$usb_interrupt_read" = "yes"); then
		usb_found=yes
	else
		usb_found=no
	fi
])

dnl AC_DEFUN([AC_PATH_GLIB], [
dnl 	PKG_CHECK_MODULES(GLIB, glib-2.0 gobject-2.0 gthread-2.0, glib_found=yes, AC_MSG_RESULT(no))
dnl 	AC_SUBST(GLIB_CFLAGS)
dnl 	AC_SUBST(GLIB_LIBS)
dnl 	GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
dnl 	AC_SUBST(GLIB_GENMARSHAL)
dnl ])

AC_DEFUN([AC_VISIBILITY], [
	case $host in
	*-*-mingw32*)
		AC_SUBST(CFLAG_VISIBILITY)
		if (test "${enable_shared}" = "yes"); then
		   OPENOBEX_CFLAGS="-DOPENOBEX_EXPORTS"
		fi
		AC_SUBST(OPENOBEX_CFLAGS)
		;;
	*)
		gl_VISIBILITY
		;;
	esac
])

AC_DEFUN([AC_ARG_OPENOBEX], [
	fortify_enable=yes
	irda_enable=yes
	bluetooth_enable=yes
	usb_enable=yes
	glib_enable=no
	apps_enable=no
	debug_enable=no
	syslog_enable=no
	dump_enable=no

	AC_ARG_ENABLE(fortify, AC_HELP_STRING([--disable-fortify], [disable compile time buffer checks]), [
		fortify_enable=${enableval}
	])

	AC_ARG_ENABLE(irda, AC_HELP_STRING([--disable-irda], [disable IrDA support]), [
		irda_enable=${enableval}
	])

	AC_ARG_ENABLE(bluetooth, AC_HELP_STRING([--disable-bluetooth], [disable Bluetooth support]), [
		bluetooth_enable=${enableval}
	])

	AC_ARG_ENABLE(usb, AC_HELP_STRING([--disable-usb], [disable USB support]), [
		usb_enable=${enableval}
	])

	dnl AC_ARG_ENABLE(glib, AC_HELP_STRING([--enable-glib], [enable GLib bindings]), [
	dnl 	glib_enable=${enableval}
	dnl ])

	AC_ARG_ENABLE(apps, AC_HELP_STRING([--enable-apps], [enable test applications]), [
		apps_enable=${enableval}
	])

	AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [enable compiling with debugging information]), [
		debug_enable=${enableval}
	])

	AC_ARG_ENABLE(syslog, AC_HELP_STRING([--enable-syslog], [enable debugging to the system logger]), [
		syslog_enable=${enableval}
	])

	AC_ARG_ENABLE(dump, AC_HELP_STRING([--enable-dump], [enable protocol dumping for debugging]), [
		dump_enable=${enableval}
	])

	if (test "${fortify_enable}" = "yes"); then
		CFLAGS="$CFLAGS -D_FORTIFY_SOURCE=2"
	fi

	REQUIRES=""

	if (test "${irda_enable}" = "yes" && test "${irda_found}" = "yes"); then
		AC_DEFINE(HAVE_IRDA, 1, [Define if system supports IrDA and it's enabled])
            if (test "${irda_windows}" = "yes"); then
                  AC_DEFINE(HAVE_IRDA_WINDOWS, 1, [Define if system supports IrDA stack for Windows])
            elif (test "${irda_linux}" = "yes"); then
		      AC_DEFINE(HAVE_IRDA_LINUX, 1, [Define if system supports IrDA stack for Linux])
            fi
	fi

      if (test "${bluetooth_enable}" = "yes" && test "${winbt_found}" = "yes"); then
            AC_DEFINE(HAVE_BLUETOOTH, 1, [Define if system supports Bluetooth and it's enabled])
            AC_DEFINE(HAVE_BLUETOOTH_WINDOWS, 1, [Define if system supports Bluetooth stack for Windows])
      fi

	if (test "${bluetooth_enable}" = "yes" && test "${netbsdbt_found}" = "yes"); then
		AC_DEFINE(HAVE_BLUETOOTH, 1, [Define if system supports Bluetooth and it's enabled])
		AC_DEFINE(HAVE_BLUETOOTH_NETBSD, 1, [Define if system supports Bluetooth stack for NetBSD])
	fi

	if (test "${bluetooth_enable}" = "yes" && test "${freebsdbt_found}" = "yes"); then
		AC_DEFINE(HAVE_BLUETOOTH, 1, [Define if system supports Bluetooth and it's enabled])
		AC_DEFINE(HAVE_BLUETOOTH_FREEBSD, 1, [Define if system supports Bluetooth stack for FreeBSD])
	fi

	if (test "${bluetooth_enable}" = "yes" && test "${bluez_found}" = "yes"); then
		AC_DEFINE(HAVE_BLUETOOTH, 1, [Define if system supports Bluetooth and it's enabled])
		AC_DEFINE(HAVE_BLUETOOTH_LINUX, 1, [Define if system supports Bluetooth stack for Linux])
	fi

	if (test "${usb_enable}" = "yes" && test "${usb_found}" = "yes"); then
		AC_DEFINE(HAVE_USB, 1, [Define if system supports USB and it's enabled])
	fi

	AM_CONDITIONAL(GLIB, test "${glib_enable}" = "yes" && test "${glib_found}" = "yes")
	AM_CONDITIONAL(APPS, test "${apps_enable}" = "yes")

	if (test "${debug_enable}" = "yes" && test "${ac_cv_prog_cc_g}" = "yes"); then
		CFLAGS="$CFLAGS -g"
		AC_DEFINE_UNQUOTED(OBEX_DEBUG, 1, [Enable debuggging])
	fi

	if (test "${syslog_enable}" = "yes"); then
		AC_DEFINE_UNQUOTED(OBEX_SYSLOG, 1, [System logger debugging])
	fi

	if (test "${dump_enable}" = "yes"); then
		AC_DEFINE_UNQUOTED(OBEX_DUMP, 1, [Protocol dumping])
	fi

	AC_SUBST(REQUIRES)
])
