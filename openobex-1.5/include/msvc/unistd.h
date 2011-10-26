/* unistd.h for Microsoft compiler
 * This replaces the standard header and provides common
 * type definitions.
 */

#ifndef _MSC_VER // [
#error "Use this header only with Microsoft Visual C++ compilers!"
#endif // _MSC_VER ]

#ifndef _MSC_UNISTD_H
#define _MSC_UNISTD_H

/* the type returned by function like _write() and send() */
#define ssize_t int

#endif /* _MSC_UNISTD_H */