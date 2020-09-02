\set client_min_messages notice
\set VERBOSITY verbose

-- This is fine, normal return path
SELECT errcontext_stack_corruption.my_func(true, 'errcontext_buggy');

-- This returns with a bogus errcontext
SELECT errcontext_stack_corruption.my_func(false, 'errcontext_buggy');

-- So calling this unrelated code will crash when it does the errcontext
-- callback... probably? It depends hugely on stack use, where things get
-- placed, etc.
--
DO LANGUAGE plpgsql
$$
BEGIN
	RAISE EXCEPTION 'foo';
END;
$$;
