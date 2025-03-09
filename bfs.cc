#include "scanner.hpp"
#include "parser.hpp"
#include "bf_space.hpp"
#include <iostream>
#include <fstream>
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
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input file> <output file>\n";
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];

    std::ifstream input(input_file);
    if (!input) {
        std::cerr << "Could not open input file: " << input_file << "\n";
        return 1;
    }

    std::string nbf_code((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    input.close();

    Scanner scanner(nbf_code.c_str());
    auto tokens = scanner.scanTokens();             
    Parser parser(tokens);
    auto functions = parser.parse();
    BfSpace bfs;
    bfs.register_functions(functions);

    std::ofstream output(output_file);
    if (!output) {
        std::cerr << "Could not open output file: " << output_file << "\n";
        return 1;
    }

    output << bfs.code() << std::endl;
    output.close();

    return 0;
}