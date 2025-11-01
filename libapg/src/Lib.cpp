#include <iostream>;
import apg_package;

void hello()
{
    const auto pkg = new ApgPackage();
    std::cout << pkg -> toString() << std::endl;
    delete pkg;
}
