#include <bits/stdc++.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

int _can_fd = -1;
typedef enum {
    CAN_PACKET_SET_DUTY = 0,      //Duty cycle mode
    CAN_PACKET_SET_CURRENT,       //Current loop mode
    CAN_PACKET_SET_CURRENT_BRAKE, // Current brake mode
    CAN_PACKET_SET_RPM,           //Velocity mode
    CAN_PACKET_SET_POS,           // Position mode
    CAN_PACKET_SET_ORIGIN_HERE,   //Set origin mode
    CAN_PACKET_SET_POS_SPD,       //Position velocity loop mode
} CAN_PACKET_ID;

void comm_can_transmit_eid(uint32_t id, const uint8_t *data, uint8_t len) {
    struct can_frame cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.can_id = id | CAN_EFF_FLAG;

    if (len > 8)
        len = 8;

    cmd.can_dlc = len;
    for(uint8_t i = 0;i < len;i++)
        cmd.data[i] = data[i];
    write(_can_fd, &cmd, sizeof(struct can_frame));
}

void buffer_append_int16(uint8_t* buffer, int16_t number, int32_t *index) {
    buffer[(*index)++] = number >> 8;
    buffer[(*index)++] = number;
}

void buffer_append_int32(uint8_t* buffer, int32_t number, int32_t *index) {
    buffer[(*index)++] = number >> 24;
    buffer[(*index)++] = number >> 16;
    buffer[(*index)++] = number >> 8;
    buffer[(*index)++] = number;
}

void comm_can_set_pos(uint8_t controller_id, float pos) {
    int32_t send_index = 0;
    uint8_t buffer[4];
    buffer_append_int32(buffer, (int32_t)(pos * 10000.0), &send_index);
    comm_can_transmit_eid(controller_id |
            ((uint32_t)CAN_PACKET_SET_POS << 8), buffer, send_index);
}

void comm_can_set_pos_spd(uint8_t controller_id, float pos,int16_t spd, int16_t RPA ) {
    int32_t send_index = 0;
    uint8_t buffer[4];
    buffer_append_int32(buffer, (int32_t)(pos * 10000.0), &send_index);
    buffer_append_int16(buffer, spd, &send_index);
    buffer_append_int16(buffer, RPA, &send_index);
    comm_can_transmit_eid(controller_id |
            ((uint32_t)CAN_PACKET_SET_POS_SPD << 8), buffer, send_index);
}

int main(){
    struct sockaddr_can addr;
    struct ifreq ifr;
    std::string _name = "can0";

    _can_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(_can_fd < 0) {
        throw "Cannot open CAN Socket!";
    };

    strcpy(ifr.ifr_name, _name.c_str());
    ioctl(_can_fd, SIOCGIFINDEX, &ifr);

    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if(setsockopt(_can_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0)
        throw ("setsockopt failed\n");

    if(bind(_can_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        throw "Failed to bind CAN!";

    comm_can_set_pos(0x001, 90.0f);
    return 0;
}
