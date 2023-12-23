#ifndef SIM800_H_
#define SIM800_H_

#include "AT_CellularDevice.h"
#include "AT_CellularSMS.h"
#include "DigitalOut.h"

namespace mbed
{

    typedef Callback<void(const char *phone, size_t phone_len, int type)> call_fnc;

    class SIM800 : public AT_CellularDevice
    {
    public:
        SIM800(FileHandle *fh, PinName pwr = NC);

        nsapi_error_t init();

        nsapi_error_t terminate_call();

        nsapi_error_t call(const char *tlf);

        void set_clip_callback(call_fnc f);

        nsapi_error_t at();

        virtual nsapi_error_t soft_power_on();
        virtual nsapi_error_t soft_power_off();

    protected: // AT_CellularDevice
        virtual nsapi_error_t get_sim_state(SimState &state);

        virtual AT_CellularContext *create_context_impl(ATHandler &at, const char *apn, bool cp_req = false, bool nonip_req = false);

        virtual nsapi_error_t shutdown();



    public: // NetworkInterface
        void handle_urc(FileHandle *fh);

    private:
        void press_button(DigitalOut &button, std::chrono::duration<uint32_t, std::milli> timeout);
        bool wake_up();

        void ring_urc();
        void clip_urc();

        DigitalOut _pwr;
        call_fnc _clip_cb;
        Callback<void()> _cring_cb;
    };
} // namespace mbed

#endif // SIM800_H_
