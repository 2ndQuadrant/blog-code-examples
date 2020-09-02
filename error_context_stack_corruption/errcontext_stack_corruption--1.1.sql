\echo Run this via CREATE EXTENSION errcontext_stack_corruption \quit

CREATE FUNCTION my_func(do_it bool, variant text)
RETURNS void RETURNS NULL ON NULL INPUT LANGUAGE c
AS 'MODULE_PATHNAME','my_func_sql';
