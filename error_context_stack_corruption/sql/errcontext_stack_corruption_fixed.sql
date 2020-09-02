\set client_min_messages notice
SELECT errcontext_stack_corruption.my_func(true, 'errcontext_fixed');
SELECT errcontext_stack_corruption.my_func(false, 'errcontext_fixed');
