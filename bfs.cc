#include "transformer.hpp"
#include "bfi.hpp"
#include <iostream>


int main(int argc, const char * argv[]) {
    char nbf_code[] = R"(
    foo:2+++++.
    bar:300--.
    foo:2++++.++++.
    )";

    nbf::Transformer t{nbf_code};
    auto bf = t.transform();

    std::cout << bf << std::endl;

    char sierpinski[] = R"(
[sierpinski.b -- display Sierpinski triangle
(c) 2016 Daniel B. Cristofani
http://brainfuck.org/]

++++++++[>+>++++<<-]>++>>+<[-[>>+<<-]+>>]>+[
    -<<<[
        ->[+[-]+>++>>>-<<]<[<]>>++++++[<<+++++>>-]+<<++.[-]<<
    ]>.>+[>>]>+
]

[Shows an ASCII representation of the Sierpinski triangle
(iteration 5).]        
    )";



    bfi::BrainfuckInterpreter interpreter(sierpinski);
    interpreter.run();

    return 0;
}