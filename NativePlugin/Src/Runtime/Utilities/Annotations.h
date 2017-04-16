#ifndef ANNOTATIONS_H
#define ANNOTATIONS_H

// Annotations for various stuff.
// At the moment only understood by Visual Studio, and expand to nothing elsewhere.

#if UNITY_WIN
#pragma warning(disable:6255) // _alloca
#pragma warning(disable:6211) // leaking due to exception
#define INPUT_NOTNULL	_In_
#define INPUT_OPTIONAL	_In_opt_
#define OUTPUT_NOTNULL	_Out_
#define OUTPUT_OPTIONAL	_Out_opt_
#define INOUT_NOTNULL	_Inout_
#define INOUT_OPTIONAL	_Inout_opt_
#define RETVAL_NOTNULL _Ret_
#define RETVAL_OPTIONAL _Ret_opt_
#define DOES_NOT_RETURN __declspec(noreturn)
#define ANALYSIS_ASSUME(x) { __analysis_assume(x); }
#define TAKES_PRINTF_ARGS(n,m)

#elif defined(__GNUC__)

#define INPUT_NOTNULL
#define INPUT_OPTIONAL
#define OUTPUT_NOTNULL
#define OUTPUT_OPTIONAL
#define INOUT_NOTNULL
#define INOUT_OPTIONAL
#define RETVAL_NOTNULL
#define RETVAL_OPTIONAL
#define DOES_NOT_RETURN __attribute__((noreturn))
#define ANALYSIS_ASSUME(x)
#define TAKES_PRINTF_ARGS(m,n) __attribute__((format(printf,m,n)))

#else

#define INPUT_NOTNULL
#define INPUT_OPTIONAL
#define OUTPUT_NOTNULL
#define OUTPUT_OPTIONAL
#define INOUT_NOTNULL
#define INOUT_OPTIONAL
#define RETVAL_NOTNULL
#define RETVAL_OPTIONAL
#define DOES_NOT_RETURN
#define ANALYSIS_ASSUME(x)
#define TAKES_PRINTF_ARGS(n,m)

#endif

#endif
