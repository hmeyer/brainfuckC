#include "transformer.hpp"
#include <iostream>


int main(int argc, const char * argv[]) {
    char nbf_code[] = R"(
    foo:2+++++.
    bar:300--.
    foo:2++++.++++.
    )";

    nbf::Transformer t{nbf_code};

    std::cout << t.transform() << std::endl;
    return 0;
}