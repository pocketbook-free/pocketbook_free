/* File llog.h - Macros for uniform logging
 * Copyright 2008 Lab126 Inc.
 * Author: saileshr@lab126.com
 */
#ifndef __LLOG_H__
#define __LLOG_H__

/* ******************************************************
 * DOCUMENTATION
 * ******************************************************
 *
 * INTRODUCTION
 * --------------
 * This header file provides logging macros.  The format of the output is
 * defined in such a way that it can be parsed easily to gather
 * data or for testing.
 *
 * There are several log levels.  The format of logging differs between
 * DEBUGx levels and other log levels.  Any non-debug log message will
 * contain the following information:
 *     timestamp
 *     component name (either process name or custom)
 *     (optional) subcomponent name
 *     message severity (crit, err etc.)
 *     message id string: A short string that identifies the type of message
 *     arguments in name/value pairs
 *     (optional) human readable information string
 *
 * LOG MESSAGES AND EVENTS
 * -----------------------
 * This log mechanism provides for logging "events".  Events are significant
 * happenings in the system that are tracked for information gathering or
 * other purposes.  Every component will get requirements for the events
 * it must log.  These events must be logged using the provided event logging
 * interface.
 *
 * Some elements of the above list are provided by syslog.  Rest need to
 * be provided by developer when logging the message.
 *
 * LOG LEVELS
 * ----------
 * The following log levels are available:
 *     LLOG_LEVEL_CRIT: An unrecoverable error. System may be unstable
 *     LLOG_LEVEL_ERROR: Serious error.  System may recover from it.
 *     LLOG_LEVEL_WARN: Unexpected situation that is handled
 *     LLOG_LEVEL_INFO: Normal but significant event
 *     LLOG_LEVEL_DEBUGx: Debug messages
 *     LLOG_LEVEL_PERF: Performance messages
 *
 * LOGGING INTERFACE OVERVIEW
 * --------------------------
 * The interface for logging any non-debug messages is similar.
 * LLOG_<LEVEL>( msg_id_str, args_format, msg_format, ... )
 * and
 * LLOGS_<LEVEL>( sub_component_str, msg_id_str, args_format, msg_format, ... )
 * where,
 * sub_component_str: Name of the subcomponent.  If you use LLOG_... interface
 * subcomponent defaults to the default subcomponent "def"
 *
 * msg_id_str: A short message id string
 *
 * arg_format: The format for the name value pairs of information that
 * goes along with the msg_id
 *
 * msg_format: A free format string for human readable message
 *
 * followed by var args
 *
 *
 * Debug messages look like:
 * LLOG_DEBUGx( msg_format, ... )
 * and
 * LLOGS_DEBUGx( sub_component_str, msg_format, ... )
 *
 * USING THE LOGGING INTERFACE
 * ---------------------------
 * A component that uses the logging interface must do the following:
 * 1. Define a variable to store the current log level (LLOG_G_LOG_MASK).
 * this variable determines that messages are logged.  The value is an
 * OR'd value of all the log levels that need to be allowed
 * This variable must be defined once per component that uses logging
 *
 * 2. Include lab126_log.h file in the component.
 *
 * 3. Call LLOG_INIT before making any other calls to logging
 *
 * 4. Use the logging interface for logging
 *
 * DETAILED INTERFACE DESCRIPTION
 * ------------------------------
 * Embedded within the header file. Look for Doxygen style comments
 * (slash and double asterisk)
 *
 */

// Include appropriate headers depending on whether we are in kernel or not
#ifndef __KERNEL__
#include <syslog.h>
#include <stdarg.h>

// If we are not in kernel, define KERN_ERR etc to avoid compiler errors
// We don't actually use these constants but are used in intermediate macros for
// ease of making this header file
#define KERN_EMERG      "<0>"   /* system is unusable */
#define KERN_ALERT      "<1>"   /* action must be taken immediately */
#define KERN_CRIT       "<2>"   /* critical conditions */
#define KERN_ERR        "<3>"   /* error conditions */
#define KERN_WARNING    "<4>"   /* warning conditions */
#define KERN_NOTICE     "<5>"   /* normal but significant condition */
#define KERN_INFO       "<6>"   /* informational */
#define KERN_DEBUG      "<7>"   /* debug-level messages */
#define KERN_PERF       "<8>"   /* performance-level messages */
#else

// FIXME: Do we need to include any headers for kernel?
// Define syslog specific stuff to avoid compiler warnings
// We don't actually use these constants but are used in intermediate macros for
// ease of making this header file
#define LOG_EMERG   0   /* system is unusable */
#define LOG_ALERT   1   /* action must be taken immediately */
#define LOG_CRIT    2   /* critical conditions */
#define LOG_ERR     3   /* error conditions */
#define LOG_WARNING 4   /* warning conditions */
#define LOG_NOTICE  5   /* normal but significant condition */
#define LOG_INFO    6   /* informational */
#define LOG_DEBUG   7   /* debug-level messages */
#define LOG_PERF    8   /* performance-level messages */
#define LOG_LOCAL0  (16<<3) /* reserved for local use */
#define LOG_LOCAL1  (17<<3) /* reserved for local use */
#define LOG_LOCAL2  (18<<3) /* reserved for local use */
#define LOG_LOCAL3  (19<<3) /* reserved for local use */
#define LOG_LOCAL4  (20<<3) /* reserved for local use */
#define LOG_LOCAL5  (21<<3) /* reserved for local use */
#define LOG_LOCAL6  (22<<3) /* reserved for local use */
#define LOG_LOCAL7  (23<<3) /* reserved for local use */

#endif

#define LLOG_MSG_ID_CRIT    "C"
#define LLOG_MSG_ID_ERR     "E"
#define LLOG_MSG_ID_WARN    "W"
#define LLOG_MSG_ID_INFO    "I"
#define LLOG_MSG_ID_DEBUG   "D"
#define LLOG_MSG_ID_EVENT   "T"
#define LLOG_MSG_ID_PERF    "P"

#ifndef LLOG_G_LOG_MASK
#define LLOG_G_LOG_MASK g_lab126_log_mask
#endif

/**
 * The following must be declared somewhere in the code
 * This variable determines which log messages are logged.
 * All log levels that need to be logged must be OR'd.
 * This variable can be changed at run time to change the log level
 * dynamically.
 */
extern unsigned int LLOG_G_LOG_MASK;

/**
 * The following are the available log levels.  They can be OR'd to form
 * a log mask
 */
#define LLOG_LEVEL_CRIT		0x04000000
#define LLOG_LEVEL_ERROR	0x02000000
#define LLOG_LEVEL_WARN		0x01000000
#define LLOG_LEVEL_INFO		0x00800000
#define	LLOG_LEVEL_EVENT	0x00200000
#define LLOG_LEVEL_DEBUG0	0x00008000
#define LLOG_LEVEL_DEBUG1	0x00004000
#define LLOG_LEVEL_DEBUG2	0x00002000
#define LLOG_LEVEL_DEBUG3	0x00001000
#define LLOG_LEVEL_DEBUG4	0x00000800
#define LLOG_LEVEL_DEBUG5	0x00000400
#define LLOG_LEVEL_DEBUG6	0x00000200
#define LLOG_LEVEL_DEBUG7	0x00000100
#define LLOG_LEVEL_DEBUG8	0x00000080
#define LLOG_LEVEL_DEBUG9	0x00000040
#define LLOG_LEVEL_PERF		0x00000020
/**
 * The following masks can be used for filter log messages
 */
#define LLOG_LEVEL_MASK_DEBUG		0x0000FFFF
#define LLOG_LEVEL_MASK_INFO		0x00FF0000
#define	LLOG_LEVEL_MASK_EXCEPTIONS	0xFF000000
#define	LLOG_LEVEL_MASK_ALL			0xFFFFFFFF

#define _LLOG_LEVEL_DEFAULT	LLOG_LEVEL_MASK_INFO|LLOG_LEVEL_MASK_EXCEPTIONS
#define _LSUBCOMP_DEFAULT "def"

/**
 * Log facilities can be dedicated for different daemons.
 * All events are logged as facility LOG_FACILITY_EVENTS
 */
#define LLOG_FACILITY_DEFAULT	LOG_LOCAL0
#define LLOG_FACILITY_EVENTS 	LOG_LOCAL1
#define LLOG_FACILITY_POWERD 	LOG_LOCAL2
#define LLOG_FACILITY_CVM		LOG_LOCAL3
#define LLOG_FACILITY_SYSTEM	LOG_LOCAL4

/**
 * LLOG_INIT must be called before using the logging interface.
 * Log mask is initialized to allow all non-debug messages.
 * Please note that the interface is different for
 * kernel and non-kernel components
 */
#ifndef __KERNEL__
#	define LLOG_INIT( str_comp_name, facility ) \
		LLOG_G_LOG_MASK = _LLOG_LEVEL_DEFAULT;\
		openlog( str_comp_name, LOG_CONS | LOG_PID, facility )
#else
#	define LLOG_INIT() \
		LLOG_G_LOG_MASK = _LLOG_LEVEL_DEFAULT
#endif


/**
 * Sets the mask to allow log messages
 */
#define LLOG_SET_MASK( new_mask ) \
	LLOG_G_LOG_MASK = new_mask

#ifndef __KERNEL__
   #define _LLOGMSG_PRINT( kern_level, syslog_level, format, ... ) \
		syslog( syslog_level, format, ##__VA_ARGS__ )
#else
   #ifndef LLOG_KERNEL_COMP
   #define LLOG_KERNEL_COMP "kernel"
   #endif
   #define _LLOGMSG_PRINT( kern_level, syslog_level, format, ... ) \
		printk( kern_level LLOG_KERNEL_COMP ": " format, ##__VA_ARGS__ )
#endif

#define _LLOGMSG_CHK( kern_level, syslog_level, log_level, format, ... ) \
		if( log_level & LLOG_G_LOG_MASK ) _LLOGMSG_PRINT( kern_level, syslog_level, format, ## __VA_ARGS__ )

#define _LLOGMSG_FORMAT( kern_level, syslog_level, log_level, str_subcomp, msg_id_char, msg_id_str, args_format, msg_format, ... ) \
	_LLOGMSG_CHK( kern_level, syslog_level, log_level, msg_id_char " " str_subcomp ":"  msg_id_str ":" args_format ":" msg_format, ## __VA_ARGS__ )

/**
 * Logs an event.  Accepts a subcomponent string
 */
#define LLOGS_EVENT( str_subcomp, str_evt_id, evt_args_format, evt_msg_format, ... ) \
	_LLOGMSG_FORMAT( KERN_INFO, LOG_INFO | LLOG_FACILITY_EVENTS, LLOG_LEVEL_EVENT, str_subcomp, LLOG_MSG_ID_EVENT, str_evt_id, evt_args_format, evt_msg_format, ## __VA_ARGS__ )

/**
 * Logs an event. Uses default subcomponent
 */
#define LLOG_EVENT( str_evt_id, evt_args_format, evt_msg_format, ... ) \
	LLOGS_EVENT( _LSUBCOMP_DEFAULT, str_evt_id, evt_args_format, evt_msg_format )

/**
 * Logs at LLOG_LEVEL_CRIT.  Accepts a subcomponent string
 */
#define LLOGS_CRIT( str_subcomp, str_error_id, err_args_format, err_msg_format, ... ) \
	    _LLOGMSG_FORMAT( KERN_CRIT, LOG_EMERG, LLOG_LEVEL_CRIT, str_subcomp, LLOG_MSG_ID_CRIT, str_error_id, err_args_format, err_msg_format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_CRIT and default subcomponent
 */
#define LLOG_CRIT( str_error_id, err_args_format, err_msg_format, ... ) \
		LLOGS_CRIT( _LSUBCOMP_DEFAULT, str_error_id, err_args_format, err_msg_format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_ERROR.  Accepts a subcomponent string
 */
#define LLOGS_ERROR( str_subcomp, str_error_id, err_args_format, err_msg_format, ... ) \
	    _LLOGMSG_FORMAT( KERN_ERR, LOG_ERR, LLOG_LEVEL_ERROR, str_subcomp, LLOG_MSG_ID_ERR, str_error_id, err_args_format, err_msg_format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_ERROR and default subcomponent
 */
#define LLOG_ERROR( str_error_id, err_args_format, err_msg_format, ... ) \
		LLOGS_ERROR( _LSUBCOMP_DEFAULT, str_error_id, err_args_format, err_msg_format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_WARN.  Accepts a subcomponent string
 */
#define LLOGS_WARN( str_subcomp, str_warn_id, warn_args_format, warn_msg_format, ... ) \
	    _LLOGMSG_FORMAT( KERN_WARNING, LOG_WARNING, LLOG_LEVEL_WARN, str_subcomp, LLOG_MSG_ID_WARN, str_warn_id, warn_args_format, warn_msg_format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_WARN and default subcomponent
 */
#define LLOG_WARN( str_warn_id, warn_args_format, warn_msg_format, ... ) \
		LLOGS_WARN( _LSUBCOMP_DEFAULT, str_warn_id, warn_args_format, warn_msg_format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_INFO.  Accepts a subcomponent string
 */
#define LLOGS_INFO( str_subcomp, str_info_id, info_args_format, info_msg_format, ... ) \
	    _LLOGMSG_FORMAT( KERN_NOTICE, LOG_NOTICE, LLOG_LEVEL_INFO, str_subcomp, LLOG_MSG_ID_INFO, str_info_id, info_args_format, info_msg_format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_INFO and default subcomponent
 */
#define LLOG_INFO( str_info_id, info_args_format, info_msg_format, ... ) \
		LLOGS_INFO( _LSUBCOMP_DEFAULT, str_info_id, info_args_format, info_msg_format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_PERF.  Accepts a subcomponent string
 */
#define LLOGS_PERF( str_subcomp, str_perf_id, perf_args_format, perf_msg_format, ... ) \
	    _LLOGMSG_FORMAT( KERN_PERF, LOG_DEBUG, LLOG_LEVEL_PERF, str_subcomp, LLOG_MSG_ID_PERF, str_perf_id, perf_args_format, perf_msg_format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_PERF and default subcomponent
 */
#define LLOG_PERF( str_perf_id, perf_args_format, perf_msg_format, ... ) \
		LLOGS_PERF( _LSUBCOMP_DEFAULT, str_perf_id, perf_args_format, perf_msg_format, ## __VA_ARGS__ )

/**
 * Logs at LLOGS_LEVEL_PERF, with absolute timing.  Accepts a subcomponent string
 */
#define LLOGS_PERF_ABSTIME( str_subcomp, str_perf_id, str_perf_time, perf_msg_format, ... ) \
		LLOGS_PERF( str_subcomp, "AbsoluteTiming", "id=" str_perf_id ",time=" str_perf_time ",type=absolute", perf_msg_format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_PERF and default subcomponent, with absolute timing
 */
#define LLOG_PERF_ABSTIME( str_perf_id, str_perf_time, perf_msg_format, ... ) \
		LLOGS_PERF_ABSTIME( _LSUBCOMP_DEFAULT, str_perf_id, str_perf_time, perf_msg_format, ## __VA_ARGS__ )

/**
 * Logs at LLOGS_LEVEL_PERF, with relative timing.  Accepts a subcomponent string
 */
#define LLOGS_PERF_RELTIME(str_subcomp, str_perf_id, str_perf_time, perf_msg_format, ... ) \
		LLOGS_PERF( str_subcomp, "RelativeTiming", "id=" str_perf_id ",time=" str_perf_time ",type=relative", perf_msg_format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_PERFand default subcomponent, with relative timing.
 */
#define LLOG_PERF_RELTIME(str_perf_id, str_perf_time, perf_msg_format, ... ) \
		LLOGS_PERF_RELTIME(_LSUBCOMP_DEFAULT, str_perf_id, str_perf_time, perf_msg_format, ## __VA_ARGS__  )


#ifndef __KERNEL__
    #define _LLOGS_DEBUG( log_level, str_subcomp, msg_format, ... ) \
		_LLOGMSG_CHK( KERN_DEBUG, LOG_DEBUG, log_level, LLOG_MSG_ID_DEBUG " " str_subcomp ":" msg_format " (%s:%s:%d)", ## __VA_ARGS__, \
		__func__, __FILE__, __LINE__ )
#else
    #define _LLOGS_DEBUG( log_level, str_subcomp, msg_format, ... ) \
		_LLOGMSG_CHK( KERN_DEBUG, LOG_DEBUG, log_level, LLOG_MSG_ID_DEBUG " " str_subcomp ":" msg_format, ## __VA_ARGS__ )
#endif

/**
 * Logs at LLOG_LEVEL_DEBUG0.  Accepts a subcomponent string
 */
#define LLOGS_DEBUG0( str_subcomp, format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG0, str_subcomp, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG0 and default subcomponent
 */
#define LLOG_DEBUG0( format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG0, _LSUBCOMP_DEFAULT, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG1.  Accepts a subcomponent string
 */
#define LLOGS_DEBUG1( str_subcomp, format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG1, str_subcomp, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG1 and default subcomponent
 */
#define LLOG_DEBUG1( format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG1, _LSUBCOMP_DEFAULT, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG2.  Accepts a subcomponent string
 */
#define LLOGS_DEBUG2( str_subcomp, format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG2, str_subcomp, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG2 and default subcomponent
 */
#define LLOG_DEBUG2( format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG2, _LSUBCOMP_DEFAULT, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG3.  Accepts a subcomponent string
 */
#define LLOGS_DEBUG3( str_subcomp, format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG3, str_subcomp, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG3 and default subcomponent
 */
#define LLOG_DEBUG3( format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG3, _LSUBCOMP_DEFAULT, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG4.  Accepts a subcomponent string
 */
#define LLOGS_DEBUG4( str_subcomp, format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG4, str_subcomp, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG4 and default subcomponent
 */
#define LLOG_DEBUG4( format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG4, _LSUBCOMP_DEFAULT, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG5.  Accepts a subcomponent string
 */
#define LLOGS_DEBUG5( str_subcomp, format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG5, str_subcomp, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG5 and default subcomponent
 */
#define LLOG_DEBUG5( format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG5, _LSUBCOMP_DEFAULT, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG6.  Accepts a subcomponent string
 */
#define LLOGS_DEBUG6( str_subcomp, format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG6, str_subcomp, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG6 and default subcomponent
 */
#define LLOG_DEBUG6( format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG6, _LSUBCOMP_DEFAULT, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG7.  Accepts a subcomponent string
 */
#define LLOGS_DEBUG7( str_subcomp, format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG7, str_subcomp, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG7 and default subcomponent
 */
#define LLOG_DEBUG7( format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG7, _LSUBCOMP_DEFAULT, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG8.  Accepts a subcomponent string
 */
#define LLOGS_DEBUG8( str_subcomp, format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG8, str_subcomp, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG8 and default subcomponent
 */
#define LLOG_DEBUG8( format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG8, _LSUBCOMP_DEFAULT, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG9.  Accepts a subcomponent string
 */
#define LLOGS_DEBUG9( str_subcomp, format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG9, str_subcomp, format, ## __VA_ARGS__ )

/**
 * Logs at LLOG_LEVEL_DEBUG9 and default subcomponent
 */
#define LLOG_DEBUG9( format, ... ) \
		_LLOGS_DEBUG( LLOG_LEVEL_DEBUG9, _LSUBCOMP_DEFAULT, format, ## __VA_ARGS__ )

/**
 * Macros for printing LTP-compatible messages
 * NOTE: to use these macros you MUST #include <linux/string.h> in your code
 */

#ifndef LTP_TEST_NAME
	#define LTP_TEST_NAME "nonamegiven"
#endif

#define _LTPMSG_PRINT( kern_level, test_case_name, instance_num, status_type, format, ... ) \
	printk( kern_level test_case_name " " instance_num " " status_type " : " format "\n", ##__VA_ARGS__ )

#define _LTPMSG_CHK( kern_level, test_case_name, instance_num, status_type, format, ... ) \
	if (strchr(test_case_name, ' ')) \
	_LTPMSG_PRINT( kern_level, "bad_name_given", "1", "BROK", "LTP_TEST_NAME (%s) should not contain spaces; source code needs to be fixed", test_case_name ); \
	else if( strlen(test_case_name) <= 0 ) \
	_LTPMSG_PRINT( kern_level, "null_name_given", "1", "BROK", "LTP_TEST_NAME was defined as null; source code needs to be fixed" ); \
	else if( strlen(instance_num) <= 0 ) \
	_LTPMSG_PRINT( kern_level, test_case_name, "1", "BROK", "instance_num was passed as null; source code needs to be fixed in line %d of file %s", __LINE__, __FILE__ ); \
	else if( strlen(status_type) <= 0 ) \
	_LTPMSG_PRINT( kern_level, test_case_name, "1", "BROK", "status_type was passed as null; source code needs to be fixed in line %d of file %s", __LINE__, __FILE__ ); \
	else if( strcmp(status_type, "PASS") && strcmp(status_type, "FAIL") && strcmp(status_type, "BROK") && strcmp(status_type, "WARN") && strcmp(status_type, "RETR") && strcmp(status_type, "INFO") && strcmp(status_type, "CONF") ) \
	_LTPMSG_PRINT( kern_level, test_case_name, "1", "BROK", "the string given for status_type (%s) is invalid; source code needs to be fixed in line %d of file %s", status_type, __LINE__, __FILE__ ); \
	else if( strlen(format) <= 0 ) \
	_LTPMSG_PRINT( kern_level, test_case_name, "1", "BROK", "message string was passed as null; source code needs to be fixed in line %d of file %s", __LINE__, __FILE__ ); \
	else  \
	_LTPMSG_PRINT( kern_level, test_case_name, instance_num, status_type, format, ## __VA_ARGS__ ); \

#define LTPMSG( instance_num, status_type, ltp_msg_format, ... ) \
	_LTPMSG_CHK( KERN_EMERG, LTP_TEST_NAME, instance_num, status_type, ltp_msg_format, ## __VA_ARGS__ )


#endif //__LLOG_H__
