\set client_min_messages notice
\set VERBOSITY verbose
SELECT errcontext_stack_corruption.my_func(true, 'simple');
SELECT errcontext_stack_corruption.my_func(false, 'simple');
