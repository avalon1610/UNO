#ifndef __UNO_DEFINES_H__
#define __UNO_DEFINES_H__

#ifdef WIN32
#define _WIN32_WINNT 0x0601
#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif
#else
#define override
#endif

#define NAMESPACE_PROLOG namespace UNO {
#define NAMESPACE_EPILOG }
#define USING_NAMESPACE using namespace UNO;
#undef DISALLOW_EVIL_CONSTRUCTORS
#define DISALLOW_EVIL_CONSTRUCTORS(Typename) \
	Typename(const Typename&); \
	void operator=(const Typename&)

#endif