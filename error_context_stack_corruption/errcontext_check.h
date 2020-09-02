#ifndef ERRCONTEXT_CHECK_H
#define ERRCONTEXT_CHECK_H

/*
 * Example code for blog article https://www.2ndquadrant.com/wp-admin/post.php?post=58103
 */

/*
 * gcc/clang/icc feature detection support
 *
 * https://clang.llvm.org/docs/LanguageExtensions.html
 * https://gcc.gnu.org/onlinedocs/cpp/_005f_005fhas_005fattribute.html
 */
#ifndef __has_attribute
  #define __has_attribute(x) 0
#endif

/*
 * pg_attribute_cleanup(cbfunc) can be used to annotate a stack (auto) variable
 * with a cleanup call that's automatically executed by the compiler on any
 * path where it leaves scope.
 *
 * This is useful for asserting that something got cleaned up.
 *
 * To allow for checks to run only on cassert, a variant
 * pgl_attribute_cleanup_cassert(cb) is provided that's only defined when using
 * --enable-cassert builds.
 *
 * Ignored when unsupported, so you can only use this to make sure that some
 * expected cleanup task happened before the variable exited the current scope,
 * you can't do anything destructor-like with it.
 *
 * This has been in gcc since forever, so we don't need any meaningful version
 * guard. clang supports it too.
 *
 * See https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html#index-cleanup-variable-attribute
 */
#if __has_attribute(cleanup)
#define pgl_attribute_cleanup(cb) __attribute__((cleanup(cb)))
#ifdef USE_ASSERT_CHECKING
#define pgl_attribute_cleanup_cassert(cb) __attribute__((cleanup(cb)))
#else
#define pgl_attribute_cleanup_cassert(cb)
#endif
#else
#define pgl_attribute_cleanup(cb)
#define pgl_attribute_cleanup_cassert(cb)
#endif

extern void check_errcontext_stack_on_return(const ErrorContextCallback * const cb);

/*
 * Use as
 *
 *     ErrorContextCallback ctx pgl_errcontext_check();
 *
 * to auto-check the entry is no longer referenced on the stack at end-of-scope.
 *
 * Doesn't check longjmp() so won't treat elog(ERROR) as a problem, but we
 * unwind automatically in that case so it's fine.
 *
 * Enabled here for both cassert and non-cassert builds because most people
 * trying out this extension won't have access to a cassert build.
 */
#define pgl_errcontext_check() pgl_attribute_cleanup(check_errcontext_stack_on_return)

/*
 * Backwards compatibility to allow errbacktrace() to be used on older postgres
 */
#if PG_VERSION_NUM < 130000 && !defined(PG_LOG_BACKTRACE)
#define errbacktrace() 0
#endif

#endif
