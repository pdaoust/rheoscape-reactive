# More comprehensive stdlib skipping
skip -rfu "^std::"
skip -rfu "^__gnu_cxx::"
skip -rfu "^__cxxabiv1::"
skip -rfu "_M_invoke"
skip -rfu "_M_manager"
skip -rfu "__invoke_impl"
skip -rfu "__call_impl"

# Skip anything with internal-looking names
skip -rfu "^.*::_.*"

# Skip std::function internals
skip -rfu ^std::function
skip -rfu ^std::_Function
skip -rfu ^std::_Function_base
skip -rfu ^std::__invoke
skip -rfu ^std::invoke

# Skip tuple/pair internals
skip -rfu ^std::tuple
skip -rfu ^std::pair
skip -rfu ^std::get

# Skip other common stdlib stepping issues
skip -rfu ^std::forward
skip -rfu ^std::move
skip -rfu ^std::_Invoke
skip -rfu ^__gnu_cxx::

# Skip compiler-generated code
skip -rfu ^.*__.*

set logging on
set logging file gdb_trace.txt
set trace-commands on