#include <iostream>
#include "myRegex.h"

int main() {
    myRegex regex("hello, World!", syntax_option_type::optimize);
    regex.inverse();
    std::cout << regex.match("!dlroW ,olleh") << std::endl;

    return 0;
}