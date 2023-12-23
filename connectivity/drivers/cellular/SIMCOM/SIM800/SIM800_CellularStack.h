#ifndef SIM800_CELLULARSTACK_H_
#define SIM800_CELLULARSTACK_H_

#include "AT_CellularStack.h"

namespace mbed {

#define SIM800_SOCKET_MAX 6
#define SIM800_CREATE_SOCKET_TIMEOUT 75000 //75 seconds
#define SIM800_SENT_BYTE_MAX 1460
#define SIM800_RECV_BYTE_MAX 1024

class SIM800_CellularStack : public AT_CellularStack {
public:
    SIM800_CellularStack(ATHandler &atHandler, int cid, nsapi_ip_stack_t stack_type, AT_CellularDevice &device);
    virtual ~SIM800_CellularStack();

protected: // NetworkStack

    virtual nsapi_error_t socket_listen(nsapi_socket_t handle, int backlog);

    virtual nsapi_error_t socket_accept(nsapi_socket_t server,
                                        nsapi_socket_t *handle, SocketAddress *address = 0);

    virtual nsapi_error_t socket_bind(nsapi_socket_t handle, const SocketAddress &address);

    virtual nsapi_error_t socket_connect(nsapi_socket_t handle, const SocketAddress &address);

protected: // AT_CellularStack

    virtual nsapi_error_t socket_stack_init();

    virtual int get_max_socket_count();

    virtual bool is_protocol_supported(nsapi_protocol_t protocol);

    virtual nsapi_error_t socket_close_impl(int sock_id);

    virtual nsapi_error_t create_socket_impl(CellularSocket *socket);

    virtual nsapi_size_or_error_t socket_sendto_impl(CellularSocket *socket, const SocketAddress &address,
                                                     const void *data, nsapi_size_t size);

    virtual nsapi_size_or_error_t socket_recvfrom_impl(CellularSocket *socket, SocketAddress *address,
                                                       void *buffer, nsapi_size_t size);

private:
    // URC handlers
    void urc_qiurc();
    void socket_closed(int sock_id);
    void socket_closed_0();

    void handle_open_socket_response(int &modem_connect_id, int &err);
};
} // namespace mbed
#endif /* SIM800_CELLULARSTACK_H_ */
