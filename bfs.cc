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
        // fun count_down(x) {
        //     if (x > 0) {
        //         var c = x;
        //         while(c > 0) {
        //             nprint(c);
        //             c = c - 1;
        //             putc(' ');
        //         }
        //         putc('\n');
        //         count_down(x-1);
        //     }
        // }

        var hello[12] = "Hello World";
        // var numbers[3] = 0, 1, 2;
        var i = 0;
        while(hello[i]) {
            putc(hello[i]);
            i = i + 1;
        }
        putc('\n');
        
        i = 1;
        hello[i] = 'a';
        hello[i+6] = 'e';
        hello[i+7] = 'l';
        hello[i*8+1] = 't';
        hello[i*8+2] = '!';
        i = 0;
        while(hello[i]) {
            putc(hello[i]);
            i = i + 1;
        }
        putc('\n');

//       count_down(20);
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