#include "SIM800_CellularInformation.h"

using namespace mbed;

SIM800_CellularInformation::SIM800_CellularInformation(ATHandler &at, AT_CellularDevice &device) : AT_CellularInformation(at, device)
{

}

nsapi_error_t SIM800_CellularInformation::get_iccid(char *buf, size_t buf_size)
{
    return _at.at_cmd_str("+CCID", "", buf, buf_size);
}
