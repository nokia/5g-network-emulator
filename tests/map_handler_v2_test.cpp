#include <cassert>
#include <fstream>
#include <string>

#include <map/map_handler.h>

int main()
{
    const std::string path = "build/tests/map_v2.json";
    std::ofstream out(path);
    out
        << "{"
        << "\"schema_version\":2,"
        << "\"cell_number\":3,"
        << "\"cell_size\":1.0,"
        << "\"map\":[[1,2,3],[4,5,6],[7,8,9]]"
        << "}";
    out.close();

    MapHandler map(path);
    assert(map.getCellNumber() == 3);
    assert(map.getCellSize() == 1.0f);
    assert(map.getMaxApothem() == 1.0f);
    assert(map.getMacroFadingValue(0.0f, 0.0f) == 5.0f);
    assert(map.getMacroFadingValue(1.0f, 0.0f) == 6.0f);
    assert(map.getMacroFadingValue(0.0f, -1.0f) == 2.0f);
    return 0;
}
