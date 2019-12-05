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

    return 0;
}