#include "scanner.hpp"
#include "parser.hpp"
#include "bf_space.hpp"
#include <iostream>

#include <cstdlib>
#include <stdexcept>
#include <execinfo.h>


void
handler()
{
    void *trace_elems[20];
    int trace_elem_count(backtrace( trace_elems, 20 ));
    char **stack_syms(backtrace_symbols( trace_elems, trace_elem_count ));
    for ( int i = 0 ; i < trace_elem_count ; ++i )
    {
        std::cout << stack_syms[i] << "\n";
    }
    free( stack_syms );

    exit(1);
}  

int main(int argc, const char * argv[]) {
//    std::set_terminate( handler );
    constexpr char nbf_code[] = R"(
        fun println(x) {
            print(x);
            putc('\n');
        }

        var pre_fib = 0;
        var fib = 1;
        while(fib < 1000) {
            print(fib);
            putc('\n');

            var t = fib;
            fib = fib + pre_fib;
            pre_fib = t;
        }
        println(fib);
    )";
    Scanner scanner(nbf_code);
    auto tokens = scanner.scanTokens();             
    Parser parser(tokens);
    auto functions = parser.parse();
    BfSpace bfs;
    bfs.register_functions(functions);
    std::cout << bfs.code() << std::endl;
    return 0;
}