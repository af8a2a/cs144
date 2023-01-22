#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    ForwardTableEntry entry;
    entry.route_prefix = route_prefix;
    entry.prefix_length = prefix_length;
    entry.next_hop = next_hop;
    entry.interface_num = interface_num;

    // ForwardTableEntry entry(route_prefix, prefix_length, next_hop, interface_num);
    _forward_table.push_back(entry);
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
        IPv4Header header = dgram.header();
    // 最长匹配长度
    uint8_t max_l = 0;
    int index = -1;
    ForwardTableEntry entry;
    int n = _forward_table.size();

    for (int i = 0; i < n; i++) {
        // 路由器搜索路由表，以找到与数据报的目的地址相匹配的路由。我们所说的"匹配"是指目的地址的最高有效`prefix_length`比特与`route_prefix`的最高有效`prefix_length`比特相同的。
        uint32_t mask = get_mask(_forward_table[i].prefix_length);
        if ((header.dst & mask) == (_forward_table[i].route_prefix & mask)) {
            // 注意最长匹配长度可能为0, 所以要通过index判断
            if ((index == -1) || (_forward_table[i].prefix_length > max_l)) {
                max_l = _forward_table[i].prefix_length;
                index = i;
                entry = _forward_table[index];
            }
        }
    }

    // 如果没有匹配的路由，路由器会丢弃数据报。
    if (index == -1) {
        return;
    }
    // 路由器会递减数据报的TTL（生存时间）。如果TTL已经为零，或在递减后为零，路由器应该放弃该数据报。
    if (dgram.header().ttl <= 1) {
        return;
    }
    dgram.header().ttl--;

    // 选择interface
    if (_forward_table[index].next_hop.has_value()) {
        // 但如果路由器是通过其他路由器连接到有关网络的，则`next_hop`将包含路径上下一路由器的IP地址。
        Address next_hop = _forward_table[index].next_hop.value();
        interface(_forward_table[index].interface_num).send_datagram(dgram, next_hop);
    } else {
        // 如果路由器直接连接到有关的网络，`next_hop`将是一个空的可选项。在这种情况下，`next_hop`是数据报的目标地址。
        Address next_hop = Address::from_ipv4_numeric(header.dst);
        // 发送
        interface(_forward_table[index].interface_num).send_datagram(dgram, next_hop);
    }

}
uint32_t Router::get_mask(uint8_t prefix_length) {
    if (prefix_length == 0) {
        return 0;
    } else {
        return ~((1 << (32 - prefix_length)) - 1);
    }
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
