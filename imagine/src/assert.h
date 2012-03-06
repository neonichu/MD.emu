#pragma once

#ifdef IMAGINE_SRC
	#include <config.h>
#endif

#if !defined(CONFIG_BASE_PS3) && !defined(CONFIG_BASE_ANDROID)
	#include_next <assert.h>
#elif !defined(NDEBUG)
	#ifdef __cplusplus
	// custom assert() implementation
	#include <logger/interface.h>
	namespace Base { void abort() ATTRS(noreturn); }

	#define assert(condition) if(!(condition)) \
	{ logger_printf(0, "assert failed: %s in " __FILE__ ", line %d , in function %s", #condition, __LINE__, __PRETTY_FUNCTION__); Base::abort(); }
	#else
	#define assert(condition) { }
	#endif
#else
	#define assert(condition) { }
#endif
