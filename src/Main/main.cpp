#include "PbrRenderer.hpp"

int main(){
    try
    {
        PbrRenderer renderer{ true };
        renderer.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}