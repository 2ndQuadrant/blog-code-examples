#include "postgres.h"
#include "errcontext_check.h"

/*
 * Example code for blog article https://www.2ndquadrant.com/wp-admin/post.php?post=58103
 */

/*
 * Check that error context stack entries don't leak if we escape scope.
 *
 * Implementation of pgl_errcontext_check()
 */
void
check_errcontext_stack_on_return(const ErrorContextCallback * const cb)
{
    ErrorContextCallback *stack;

    /*
     * No reference to the stack-allocated ErrorContextCallback that's leaving
     * scope may remain on the global error_context_stack.
     *
     * There doesn't seem to be a good lightweight way to acquire the caller
     * function name; we can't use clang's __builtin_FUNCTION() etc because we
     * can't supply default-parameters.
     */
    for (stack = error_context_stack; stack != NULL; stack = stack->previous)
    {
        if (stack == cb)
        {
            /*
             * Fix the error context stack and ERROR the backend. If the postgres
             * version supports it, emit a backtrace for the error to the log
             * as well.
             *
             * A more useful implementation for real world use would PANIC here
             * or would use pgl_errcontext_check_cassert() and would then
             * Assert(stack != cb). But in this case we want to keep the backend
             * alive, so we don't have to restart it.
             */
            error_context_stack = stack->previous;
            ereport(ERROR,
                    (errcode(ERRCODE_INTERNAL_ERROR),
                     errmsg_internal("leaked an errcontext callback pointer"),
                     errbacktrace()));
        }
    }
}
