PGFILEDESC = "errcontext_stack_corruption - blog example"
MODULES = errcontext_stack_corruption
EXTENSION = errcontext_stack_corruption
DATA = errcontext_stack_corruption--1.0.sql

REGRESS = \
	errcontext_stack_corruption_init \
	errcontext_stack_corruption_basic \
	errcontext_stack_corruption_fixed \
	errcontext_stack_corruption_buggy

# The errors demonstrated in this blog will NOT be caught by stack-guard logic
# like:
#
#PG_CPPFLAGS += -fstack-protector -fstack-protector-all  -fstack-protector-strong

# LLVM AddressSanitizer won't help either because it expects to run on the whole
# postgres, not just our extension.
#
#ifeq ($(USE_ADDRESS_SANITIZER),1)
#PG_CPPFLAGS += -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer
#PG_LDFLAGS += -fsanitize=address -shared-libasan -rpath
#export ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1
#endif

subdir=error_context_stack_corruption
include ../Makefile.common

check: regresscheck
