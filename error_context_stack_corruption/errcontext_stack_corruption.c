/*
 * Example code for blog article
 *
 * Craig Ringer <craig.ringer@2ndquadrant.com>
 * https://www.2ndquadrant.com/en/blog/dev-corner-error-context-stack-corruption/
 */

#include "postgres.h"

#include "funcapi.h"
#include "utils/builtins.h"

#include "errcontext_check.h"

PG_MODULE_MAGIC;

/*
 * Dummy code
 */
static void
do_the_thing(void)
{
    ereport(INFO,
            (errmsg("did the thing")));
};

/*
 * Example 1
 */
static void
my_func(bool do_it)
{
    if (!do_it)
    {
        elog(WARNING, "not doing it!");
        return;
    }

    do_the_thing();
}

/*
 * Example 2
 */
struct my_func_ctx_arg
{
    bool do_it;
};

static void
my_func_ctx_callback(void *arg)
{
    struct my_func_ctx_arg *ctx_arg = arg;
    errcontext("during my_func(do_it=%d)", ctx_arg->do_it);
}

static ErrorContextCallback *leaked_errcontext_ptr;

/*
 * WARNING WARNING WARNING
 *
 * This code is deliberately BUGGY. DO NOT COPY IT.
 */
static void
my_func_with_errcontext_BUGGY(bool do_it)
{
    ErrorContextCallback myerrcontext;
    struct my_func_ctx_arg ctxinfo;

    ctxinfo.do_it = do_it;
    myerrcontext.callback = my_func_ctx_callback;
    myerrcontext.arg = &ctxinfo;
    myerrcontext.previous = error_context_stack;
    error_context_stack = &myerrcontext;

    if (!do_it)
    {
        /*
         * -----------------------
         * WARNING WARNING WARNING
         * -----------------------
         *
         * This code is deliberately BUGGY. DO NOT COPY IT.
         */
        elog(LOG, "leaking error context stack pointer %p", error_context_stack);
        leaked_errcontext_ptr = error_context_stack;
        elog(WARNING, "not doing it and leaking the error context callback pointer!");
        return;
    }

    do_the_thing();

    /*
     * Note that this isn't necessarily strictly correct for complex functions
     * that may involve transaction management, etc:
     */
    Assert(error_context_stack == &myerrcontext);
    error_context_stack = myerrcontext.previous;
}

static void
my_func_with_errcontext_fixed(bool do_it)
{
    ErrorContextCallback myerrcontext;
    struct my_func_ctx_arg ctxinfo;

    ctxinfo.do_it = do_it;
    myerrcontext.callback = my_func_ctx_callback;
    myerrcontext.arg = &ctxinfo;
    myerrcontext.previous = error_context_stack;
    error_context_stack = &myerrcontext;

    if (!do_it)
    {
        elog(INFO, "not doing it (correctly)");
        /* don't shortcut 'return', fall through */
    }
    else
    {
        do_the_thing();
    }

    /* Safer way to unwind errcontext stack */
    if (error_context_stack == &myerrcontext)
        error_context_stack = myerrcontext.previous;
}

/*
 * WARNING WARNING WARNING
 *
 * This code is deliberately BUGGY. DO NOT COPY IT.
 *
 * This modifies my_func_with_errcontext_BUGGY very minimally to
 * add the pgl_errcontext_check() attribute on the declaration
 * of the ErrorContextCallback variable.
 *
 * It will fix the problem and ERROR when called, preventing a crash, but only
 * if the compiler supports __attribute__((cleanup)).
 */
static void
my_func_with_errcontext_ERRDETECT(bool do_it)
{
    ErrorContextCallback myerrcontext pgl_errcontext_check();
    struct my_func_ctx_arg ctxinfo;

    ctxinfo.do_it = do_it;
    myerrcontext.callback = my_func_ctx_callback;
    myerrcontext.arg = &ctxinfo;
    myerrcontext.previous = error_context_stack;
    error_context_stack = &myerrcontext;

    if (!do_it)
    {
        /*
         * -----------------------
         * WARNING WARNING WARNING
         * -----------------------
         *
         * This code is deliberately BUGGY. DO NOT COPY IT.
         */
        elog(LOG, "leaking error context stack pointer %p", error_context_stack);
        leaked_errcontext_ptr = error_context_stack;
        elog(WARNING, "not doing it and leaking the error context callback pointer!");
        return;
    }

    do_the_thing();

    /*
     * Note that this isn't necessarily strictly correct for complex functions
     * that may involve transaction management, etc:
     */
    if (error_context_stack == &myerrcontext)
        error_context_stack = myerrcontext.previous;
}

/*
 * Wrappers to make the examples SQL-callable and more reliably trigger the
 * crash.
 *
 * They rely on attributes to control compilation and prevent inlining, but
 * you can probably just use -O0 instead.
 */

#if __has_attribute(noinline)
#define ext_noinline() __attribute__((noinline))
#else
#define ext_noinline()
#endif
#if __has_attribute(optimize)
#define ext_noopt() __attribute__((optimize(0)))
#else
#define ext_noopt()
#endif

ext_noinline() ext_noopt() static void
clobber_the_stack(void)
{
    /*
     * Write a chunk of nonsense into the stack so that any space for the
     * error_context_stack gets overwritten by nonsense.
     */
#define N_DUMMY_BYTES 2000
    char dummy_bytes[N_DUMMY_BYTES];
    memset(&dummy_bytes[0], '\x7f', N_DUMMY_BYTES);
#undef N_DUMMY_BYTES
}

ext_noinline() ext_noopt() static void
call_with_padded_stack(bool do_it, const char *variant)
{
    /*
     * We pad the stack with some nonsense, but not as much as will
     * get written by the clobber_the_stack() call. This ensures that when
     * we call ereport(), the stack for errstart/errfinish don't overwrite
     * the stack space our eventual pointer into error_context_stack gets
     * put in.
     */
#define N_DUMMY_BYTES 1000
    char dummy_bytes[N_DUMMY_BYTES];
    memset(&dummy_bytes[0], '\x6f', N_DUMMY_BYTES);
#undef N_DUMMY_BYTES

    if (strcmp(variant, "simple") == 0)
        my_func(do_it);
    else if (strcmp(variant, "errcontext_buggy") == 0)
        my_func_with_errcontext_BUGGY(do_it);
    else if (strcmp(variant, "errcontext_fixed") == 0)
        my_func_with_errcontext_fixed(do_it);
    else if (strcmp(variant, "errcontext_detect") == 0)
        my_func_with_errcontext_ERRDETECT(do_it);
    else
        ereport(ERROR,
                (errmsg("unrecognised \"variant\" argument: %s", variant)));
}

PG_FUNCTION_INFO_V1(my_func_sql);

Datum
my_func_sql(PG_FUNCTION_ARGS)
{
    bool do_it = PG_GETARG_BOOL(0);
    const char * variant = text_to_cstring(PG_GETARG_TEXT_P(1));

    leaked_errcontext_ptr = NULL;

    call_with_padded_stack(do_it, variant);
    clobber_the_stack();

    ereport(INFO,
            (errmsg("after return from requested call")));

    leaked_errcontext_ptr = NULL;

    PG_RETURN_VOID();
}
