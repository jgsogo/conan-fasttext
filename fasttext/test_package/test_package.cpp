#include <iostream>
#include <fasttext/fasttext.h>

int main(void)
{
    std::cout << "Fasttext test_package\n";
    fasttext::FastText model{};
    std::cout << "FastText::isQuant() = " << model.isQuant() << "\n";
    return 0;
}