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
        var pre_fib = 0;
        var fib = 1;
        while(fib < 1000) {
            print(fib);
            if (fib == 8) putc('+');
            if (fib != 8) putc('-');
            putc('\n');

            var t = fib;
            fib = fib + pre_fib;
            pre_fib = t;
        }
    )";
    Scanner scanner(nbf_code);
    auto tokens = scanner.scanTokens();             
    Parser parser(tokens);
    auto statements = parser.parse();
    BfSpace bfs;
    for (const auto& s : statements) {
        s->evaluate(&bfs);
    }
    std::cout << bfs.code() << std::endl;
    return 0;
}