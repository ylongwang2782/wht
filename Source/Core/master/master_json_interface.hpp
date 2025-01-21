#include <stdint.h>
#include "bsp_allocate.hpp"
#include <nlohmann/json.hpp>
using json = nlohmann::json;
class JsonInterface{
public:
    JsonInterface();
    json data;
    void parseJson(const char *data);
};