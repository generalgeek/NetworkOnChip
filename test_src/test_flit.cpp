#include "flit.h"
#include <iostream>

int main(int argc, char const* argv[]) {
    Flit flit(flit_t(12345678), 910);
    std::cout << "Flit data: " << flit.getData() << std::endl;
    std::cout << "Flit packet id: " << flit.getPacketId() << std::endl;
    std::cout << "Flit unique id: " << flit.getUniqueId() << std::endl;
    flit.setData(flit_t(88888888));
    std::cout << "Flit data: " << flit.getData() << std::endl;
    std::cout << "Flit packet id: " << flit.getPacketId() << std::endl;
    std::cout << "Flit unique id: " << flit.getUniqueId() << std::endl;
    return 0;
}
