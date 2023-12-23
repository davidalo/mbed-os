#include "AT_CellularNetwork.h"
#include "SIM800_CellularContext.h"
#include "SIM800.h"
#include "AT_CellularSMS.h"
#include "ThisThread.h"

#include "CellularLog.h"

using namespace mbed;
using std::milli;
using namespace std::chrono;

#include "mbed_trace.h"
#define TRACE_GROUP "CELL"

#if !defined(MBED_CONF_SIM800_CME_ERROR)
#define MBED_CONF_SIM800_CME_ERROR 2
#endif

#define SIM800_CALL_TIMEOUT_MS 1000
#define SIM800_TERMINATE_TIMEOUT_MS 5000

static const intptr_t cellular_properties[AT_CellularDevice::PROPERTY_MAX] = {
    AT_CellularNetwork::RegistrationModeDisable,// C_EREG
    AT_CellularNetwork::RegistrationModeLAC,    // C_GREG
    AT_CellularNetwork::RegistrationModeDisable,// C_REG
    1,  // AT_CGSN_WITH_TYPE
    1,  // AT_CGDATA
    0,  // AT_CGAUTH
    1,  // AT_CNMI
    1,  // AT_CSMP
    1,  // AT_CMGF
    1,  // AT_CSDH
    1,  // PROPERTY_IPV4_STACK
    0,  // PROPERTY_IPV6_STACK
    0,  // PROPERTY_IPV4V6_STACK
    0,  // PROPERTY_NON_IP_PDP_TYPE
    1,  // PROPERTY_AT_CGEREP
};

SIM800::SIM800(FileHandle *fh, PinName pwr) : AT_CellularDevice(fh), _clip_cb(NULL), _cring_cb(NULL), _pwr(pwr)
{
    AT_CellularDevice::set_cellular_properties(cellular_properties);
}

nsapi_error_t SIM800::init() {
    _at.lock();
    _at.flush();
    _at.cmd_start("ATE0");
    _at.cmd_stop_read_resp();

    _at.cmd_start_stop("+CMEE", "=", "%d", MBED_CONF_SIM800_CME_ERROR);
    _at.resp_start();
    _at.resp_stop();

    _at.cmd_start_stop("+CFUN", "=", "%d", 1);
    _at.resp_start();
    _at.resp_stop();

    // Configure +CLIP notification
    _at.cmd_start("AT+CLIP=1");
    _at.cmd_stop_read_resp();

    // Change +RING to +CRING notification
    _at.cmd_start("AT+CRC=0");
    _at.cmd_stop_read_resp();

    _at.set_urc_handler("+CLIP:", callback(this, &SIM800::clip_urc));
    _at.set_urc_handler("RING", callback(this, &SIM800::ring_urc));

    return _at.unlock_return_error();
}

void SIM800::clip_urc()
{
    //+CLIP: tlf,
    char tlf[22] = {0};
    int type;

    tr_debug("CLIP called");
    _at.read_string(tlf, sizeof(tlf) - 1);
    type = _at.read_int(); // 129 unknow, 161 national, 145 international, 177 network specific

    tr_info("Call from: %s", tlf);

    // call user defined callback function
    if (_clip_cb) {
        _clip_cb(tlf, sizeof(tlf), type);
    } else {
        tr_warn("clip_urc, no user defined callback for receiving call!");
    }
}

void SIM800::ring_urc()
{
    // RING,
    tr_debug("RING called");

    // call user defined callback function
    if (_cring_cb) {
        _cring_cb();
    } else {
        tr_warn("cring_urc, no user defined callback for receiving voice!");
    }
}

void SIM800::set_clip_callback(call_fnc f)
{
    _clip_cb = f;
}


nsapi_error_t SIM800::get_sim_state(SimState &state)
{
    char buf[13];

    _at.lock();
    nsapi_error_t err = _at.at_cmd_str("+CPIN", "?", buf, 13);
    tr_debug("CPIN: %s", buf);
    _at.unlock();

    if (memcmp(buf, "READY", 5) == 0) {
        state = SimStateReady;
    } else if (memcmp(buf, "SIM PIN", 7) == 0) {
        state = SimStatePinNeeded;
    } else if (memcmp(buf, "SIM PUK", 7) == 0) {
        state = SimStatePukNeeded;
    } else if (memcmp(buf, "PH_SIM PIN", 10) == 0) {
        state = SimStatePinNeeded;
    } else if (memcmp(buf, "PH_SIM PUK", 10) == 0) {
        state = SimStatePukNeeded;
    } else if (memcmp(buf, "SIM PIN2", 8) == 0) {
        state = SimStatePinNeeded;
    } else if (memcmp(buf, "SIM PUK2", 8) == 0) {
        state = SimStatePukNeeded;
    } else {
        state = SimStateUnknown; // SIM may not be ready yet
    }

    return err;
}

AT_CellularContext *SIM800::create_context_impl(ATHandler &at, const char *apn, bool cp_req, bool nonip_req)
{
    return new SIM800_CellularContext(at, this, apn, cp_req, nonip_req);
}

nsapi_error_t SIM800::shutdown()
{
    return _at.at_cmd_discard("+QPOWD", "=0");
}


#if MBED_CONF_SIM800_PROVIDE_DEFAULT
#include "drivers/BufferedSerial.h"
CellularDevice *CellularDevice::get_default_instance()
{
    tr_debug("Get default instance");
    static BufferedSerial serial(MBED_CONF_SIM800_TX, MBED_CONF_SIM800_RX, MBED_CONF_SIM800_BAUDRATE);
    tr_debug("Buffered");
#if defined (MBED_CONF_SIM800_RTS) && defined(MBED_CONF_SIM800_CTS)
    tr_debug("SIM800 flow control: RTS %d CTS %d", MBED_CONF_SIM800_RTS, MBED_CONF_SIM800_CTS);
    serial.set_flow_control(SerialBase::RTSCTS, MBED_CONF_SIM800_RTS, MBED_CONF_SIM800_CTS);
#endif
    static SIM800 device(&serial,
                          MBED_CONF_SIM800_PWR);
    tr_debug("Return instance");
    return &device;
}
#endif

nsapi_error_t SIM800::call(const char *tlf) {
    char cmd[32];
    int pos = 0;

    strcpy(cmd, "ATD");
    pos = strlen("ATD");
    strncat(cmd, tlf, 32 - pos);
    pos += strlen(tlf);
    strncat(cmd, ";", 32 - pos);

    _at.lock();
    _at.set_at_timeout(SIM800_CALL_TIMEOUT_MS);
    _at.cmd_start(cmd);
    _at.cmd_stop_read_resp();
    _at.restore_at_timeout();

    return _at.unlock_return_error();
}

nsapi_error_t SIM800::terminate_call() {
    _at.lock();
    _at.set_at_timeout(SIM800_TERMINATE_TIMEOUT_MS);
    _at.cmd_start("ATH");
    _at.cmd_stop_read_resp();
    _at.restore_at_timeout();

    _at.flush();
    is_ready();

    return _at.unlock_return_error();
}

nsapi_error_t SIM800::at() {
    _at.lock();
    _at.cmd_start("AT");
    _at.cmd_stop_read_resp();

    return _at.unlock_return_error();
}


nsapi_error_t SIM800::soft_power_on()
{
    if (_pwr.is_connected()) {
        tr_info("SIM800::soft_power_on");
        // check if modem was powered on already
        if (!wake_up()) {
            tr_error("Modem not responding");
            soft_power_off();
            return NSAPI_ERROR_DEVICE_ERROR;
        }
    }

    return NSAPI_ERROR_OK;
}

nsapi_error_t SIM800::soft_power_off()
{
    _at.lock();
    _at.cmd_start("AT+QPOWD");
    _at.cmd_stop_read_resp();
    if (_at.get_last_error() != NSAPI_ERROR_OK) {
        tr_warn("Force modem off");
        if (_pwr.is_connected()) {
            press_button(_pwr, 1s);
        }
    }
    return _at.unlock_return_error();
}

bool SIM800::wake_up()
{
    // check if modem is already ready
    nsapi_error_t err = at();

    // modem is not responding, power it on
    if (err != NSAPI_ERROR_OK) {
        press_button(_pwr, 1s); // BG96_Hardware_Design_V1.1 requires time 100 ms, but 250 ms seems to be more robust

        _at.lock();
        // According to BG96_Hardware_Design_V1.1 USB is active after 4.2s, but it seems to take over 5s
        _at.set_at_timeout(6s);
        _at.resp_start();
        _at.set_stop_tag("RDY");
        bool rdy = _at.consume_to_stop_tag();
        _at.set_stop_tag(OK);
        _at.restore_at_timeout();
        _at.unlock();

        err = at();
    }

    // sync to check that AT is really responsive and to clear garbage
    return (err == NSAPI_ERROR_OK);
}

void SIM800::press_button(DigitalOut &button, duration<uint32_t, milli> timeout)
{
    if (!button.is_connected()) {
        return;
    }

    button = true;
    rtos::ThisThread::sleep_for(timeout);
    button = false;
}
