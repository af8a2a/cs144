#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}
bool NetworkInterface::equal(const EthernetAddress &d1, const EthernetAddress &d2) {
    for (int i = 0; i < 6; i++) {
        if (d1[i] != d2[i]) {
            return false;
        }
    }

    return true;
}
EthernetFrame NetworkInterface::broadcast_frame(uint32_t ip) {
    ARPMessage arp_msg;
    arp_msg.opcode = ARPMessage::OPCODE_REQUEST;
    arp_msg.sender_ethernet_address = _ethernet_address;
    arp_msg.sender_ip_address = _ip_address.ipv4_numeric();
    arp_msg.target_ethernet_address = {};
    arp_msg.target_ip_address = ip;

    EthernetHeader header;
    header.src = _ethernet_address;
    header.dst = ETHERNET_BROADCAST;
    header.type = header.TYPE_ARP;

    EthernetFrame frame;
    frame.header() = header;
    frame.payload() = arp_msg.serialize();

    return frame;
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    auto it = _arp_map.find(next_hop_ip);
    if (it == _arp_map.end()) {
        // 不在arp表中, 缓存, 等待回复后发送
        cache.push_back({next_hop, dgram});
        // 不在已发送未回复则广播, 不要重复广播
        if (waiting_msg.find(next_hop_ip) == waiting_msg.end()) {
            EthernetFrame frame = broadcast_frame(next_hop_ip);
            // 发送
            _frames_out.push(frame);
            // 更新表
            waiting_msg[next_hop_ip] = _time;
        }
    } else {
        // 在arp表中, 则直接发送
        EthernetHeader header;
        header.src = _ethernet_address;
        header.dst = (it->second).first;
        header.type = EthernetHeader::TYPE_IPv4;
        EthernetFrame frame;
        frame.header() = header;
        frame.payload() = dgram.serialize();
        _frames_out.push(frame);
    }

    
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
        EthernetHeader header = frame.header();
    // 只接受以太网目的地是广播地址或存储在以太网地址成员变量`_ethernet_address`中的以太网地址
    if (!equal(header.dst, ETHERNET_BROADCAST) && !equal(header.dst, _ethernet_address)) {
        return {};
    }

    if (header.type == header.TYPE_IPv4) {
        // 如果入站帧是IPv4，将有效载荷解析为`InternetDatagram`，如果成功（意味着`parse()`方法返回`ParseResult::NoError`），则将生成的`InternetDatagram`返回给调用者。
        InternetDatagram ip_datagram;
        ParseResult res = ip_datagram.parse(frame.payload());
        if (res == ParseResult::NoError) {
            return ip_datagram;
        } else {
            return {};
        }
    } else {
        // 如果入站帧是ARP，将有效载荷解析为ARP消息，如果成功，记住发送方的IP地址和以太网地址之间的映射，持续30秒。（从请求和回复中学习映射。）
        ARPMessage arp_msg;
        ParseResult res = arp_msg.parse(frame.payload());
        if (res == ParseResult::NoError) {
            // 发送方以太网地址和ip地址
            EthernetAddress eth_addr = arp_msg.sender_ethernet_address;
            uint32_t ip_addr = arp_msg.sender_ip_address;
            // 此外，如果是ARP请求请求我们的IP地址，请发送适当的ARP回复。
            if ((arp_msg.opcode == ARPMessage::OPCODE_REQUEST) && (arp_msg.target_ip_address == _ip_address.ipv4_numeric())) {
                EthernetHeader header_send;
                header_send.type = header_send.TYPE_ARP;
                header_send.dst = arp_msg.sender_ethernet_address;
                header_send.src = _ethernet_address;

                ARPMessage arp_msg_send;
                arp_msg_send.opcode = arp_msg_send.OPCODE_REPLY;
                arp_msg_send.sender_ethernet_address = _ethernet_address;
                arp_msg_send.sender_ip_address = _ip_address.ipv4_numeric();
                arp_msg_send.target_ethernet_address = arp_msg.sender_ethernet_address;
                arp_msg_send.target_ip_address = arp_msg.sender_ip_address;

                // 发送
                EthernetFrame frame_send;
                frame_send.header() = header_send;
                frame_send.payload() = arp_msg_send.serialize();

                _frames_out.push(frame_send);
            }
            // 更新映射
            _arp_map[ip_addr] = {eth_addr, _time};
            // 发送
            for (auto it = cache.begin(); it != cache.end();) {
                Address addr_cache = it->first;
                InternetDatagram dgram_cache = it->second;
                // 如果ip是更新的ip, 则发送
                if (addr_cache.ipv4_numeric() == ip_addr) {
                    send_datagram(dgram_cache, addr_cache);
                    // 删除
                    cache.erase(it++);
                } else {
                    it++;
                }
            }
            // 删除已发送未回应的部分
            waiting_msg.erase(ip_addr);
        }
        return {};
    }

}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { 
        _time += ms_since_last_tick;
    // 更新映射表
    for (auto it = _arp_map.begin(); it != _arp_map.end();) {
        // 超过30秒则删除
        if (_time - (it->second).second >= 30 * 1000) {
            _arp_map.erase(it++);
        } else {
            it++;
        }
    }
    // 更新已发送, 未回应的部分
    for (auto it = waiting_msg.begin(); it != waiting_msg.end(); it++) {
        // 超过5秒则重发
        if (_time - it->second >= 5 * 1000) {
            // 不在arp表中, 广播
            EthernetFrame frame = broadcast_frame(it->first);
            // 发送
            _frames_out.push(frame);
            // 更新发送时间
            it->second = _time;
        }
    }

 }
