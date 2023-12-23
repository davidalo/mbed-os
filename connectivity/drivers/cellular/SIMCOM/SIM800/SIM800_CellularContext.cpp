#include "SIM800_CellularContext.h"
#include "SIM800_CellularStack.h"
#include "SIM800.h"
#include "CellularLog.h"

namespace mbed {

SIM800_CellularContext::SIM800_CellularContext(ATHandler &at, CellularDevice *device, const char *apn, bool cp_req, bool nonip_req) :
    AT_CellularContext(at, device, apn, cp_req, nonip_req)
{
}

#if !NSAPI_PPP_AVAILABLE
NetworkStack *SIM800_CellularContext::get_stack()
{
    if (!_stack) {
        _stack = new SIM800_CellularStack(_at, _cid, (nsapi_ip_stack_t)_pdp_type, *get_device());
    }
    return _stack;
}
#endif // #if !NSAPI_PPP_AVAILABLE

nsapi_error_t SIM800_CellularContext::do_user_authentication()
{
    nsapi_error_t err = NSAPI_ERROR_OK;
    if (_pwd && _uname) {
        err = _at.at_cmd_discard("^SISO", "=", "%d%s%s%s", 1, _apn, _uname, _pwd);
    } else {
        err = _at.at_cmd_discard("^SISO", "=", "%d%s", 1, _apn);
    }
    if (err != NSAPI_ERROR_OK) {
        return NSAPI_ERROR_AUTH_FAILURE;
    }

    return err;
}

void SIM800_CellularContext::set_cid(int cid)
{
    _cid = cid;
    if (_stack) {
        static_cast<AT_CellularStack *>(_stack)->set_cid(_cid);
    }
}

} /* namespace mbed */