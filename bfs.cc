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
    //std::set_terminate( handler );
    constexpr char nbf_code[] = R"(
        fun count_down(x) {
            if (x > 0) {
                var c = x;
                while(c > 0) {
                    nprint(c);
                    c = c - 1;
                    putc(' ');
                }
                putc('\n');
                count_down(x-1);
            }
        }

        count_down(20);
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