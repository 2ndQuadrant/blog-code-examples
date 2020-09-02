\set client_min_messages notice

-- This is fine, normal return path
SELECT errcontext_stack_corruption.my_func(true, 'errcontext_detect');

-- This returns with a bogus errcontext, but if the compiler supports
-- the cleanup attribute it'll detect and clean up the problem instead
-- of corrupting the error context stack.
SELECT errcontext_stack_corruption.my_func(false, 'errcontext_detect');

-- Calling this should't crash if the compiler did the cleanup above
--
DO LANGUAGE plpgsql
$$
BEGIN
	RAISE EXCEPTION 'foo';
END;
$$;
