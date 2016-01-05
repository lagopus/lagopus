/* %COPYRIGHT% */

#define TAP_POLL_TIMEOUT 1

struct dp_tap_interface;

lagopus_result_t
dp_tap_interface_create(const char *name, struct interface *ifp);
dp_tap_interface_destroy(const char *name);
lagopus_result_t
dp_tap_start_interface(const char *name);
lagopus_result_t
dp_tap_stop_interface(const char *name);
lagopus_result_t
dp_tap_interface_send_packet(struct dp_tap_interface *tap,
                             struct lagopus_packet *pkt);
ssize_t
dp_tap_interface_recv_packet(struct dp_tap_interface *tap,
                             struct lagopus_packet *pkt);
lagopus_result_t
dp_tap_thread_loop(__UNUSED const lagopus_thread_t *selfptr,
                   __UNUSED void *arg);
