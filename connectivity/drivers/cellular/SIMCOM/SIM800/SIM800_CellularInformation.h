#ifndef SIM800_CELLULAR_INFORMATION_H_
#define SIM800_CELLULAR_INFORMATION_H_

#include "AT_CellularInformation.h"

namespace mbed {

class SIM800_CellularInformation : public AT_CellularInformation {
public:
    SIM800_CellularInformation(ATHandler &at, AT_CellularDevice &device);

public: //from CellularInformation
    virtual nsapi_error_t get_iccid(char *buf, size_t buf_size);
};

} // namespace mbed

#endif // SIM800_CELLULAR_INFORMATION_H_
