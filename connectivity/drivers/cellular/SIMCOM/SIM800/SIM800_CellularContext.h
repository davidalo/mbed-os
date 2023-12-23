#ifndef SIM800_CELLULARCONTEXT_H_
#define SIM800_CELLULARCONTEXT_H_

#include "AT_CellularContext.h"
#include "AT_CellularSMS.h"

namespace mbed {

class SIM800_CellularContext: public AT_CellularContext {
public:
    SIM800_CellularContext(ATHandler &at, CellularDevice *device, const char *apn, bool cp_req = false, bool nonip_req = false);

protected:
#if !NSAPI_PPP_AVAILABLE
    virtual NetworkStack *get_stack();
#endif // #if !NSAPI_PPP_AVAILABLE
    virtual nsapi_error_t do_user_authentication();

private:
    void set_cid(int cid);
};

} /* namespace mbed */

#endif // SIM800_CELLULARCONTEXT_H_
