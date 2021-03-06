
#include "PacketListPane.h"
#include "protocol.h"
#include "TextPane.h"

Packet::~Packet()
{
  for (size_t i=0; i<details.size(); i++) {
    delete details[i];
  }
}

void Packet::analyze(const uint8_t* ptr, size_t len)
{
  using namespace slankdev;
  details.clear();

  Ethernet* eth = new Ethernet(ptr, len);
  details.push_back(eth);
  len -= eth->headerlen();
  ptr += eth->headerlen();
  uint16_t type = eth->type();
  src = eth->src();
  dst = eth->dst();
  protocol = "Ether";
  summary  = fs("Ethernet packet.type=0x%04x", eth->type());

  switch (type) {
    case 0x800:
      {
        IP* ip = new IP(ptr, len);
        details.push_back(ip);
        len -= ip->headerlen();
        ptr += ip->headerlen();
        uint8_t proto = ip->protocol();
        src = ip->src();
        dst = ip->dst();
        protocol = "IPv4";
        summary = fs("protocol=%u\n", ip->protocol());

        switch (proto) {
          case 1:
            {
              ICMP* icmp = new ICMP(ptr, len);
              details.push_back(icmp);
              len -= icmp->headerlen();
              ptr += icmp->headerlen();
              protocol = "ICMP";
              summary = fs("type=%u code=%u", icmp->type(), icmp->code());
              break;
            }
          case 17:
            {
              UDP* udp = new UDP(ptr, len);
              details.push_back(udp);
              len -= udp->headerlen();
              ptr += udp->headerlen();
              protocol = "UDP";
              summary = fs("%u -> %u", udp->src(), udp->dst());
              break;
            }
          case 6:
            {
              TCP* tcp = new TCP(ptr, len);
              details.push_back(tcp);
              len -= tcp->headerlen();
              ptr += tcp->headerlen();
              protocol = "TCP";
              summary = fs("%u -> %u", tcp->src(), tcp->dst());
              break;
            }
        }
        break;
      }
    case 0x86dd:
      {
        /*
         * Analyze IPv6
         */
        protocol = "IPv6";
        break;
      }
    case 0x0806:
      {
        /*
         * Analyze ARP
         */
        ARP* arp = new ARP(ptr, len);
        details.push_back(arp);
        len -= arp->headerlen();
        ptr += arp->headerlen();
        protocol = "ARP";
        break;
      }
  }
  if (len > 0) details.push_back(new Binary(ptr, len));
}


Packet::Packet(const void* p, size_t l, uint64_t t, size_t n)
  : len(l), time(t),  number(n)
{
  /*
   * Init Detail Pane Information
   */
  analyze(reinterpret_cast<const uint8_t*>(p), l);

  /*
   * Init Binary Pane Information
   */
  binarys.clear();
  using namespace slankdev;

  const void* buffer = p;
  ssize_t bufferlen  = l;

  const uint8_t *data = reinterpret_cast<const uint8_t*>(buffer);
  size_t row = 0;

  std::string line;
  while (bufferlen > 0) {
    line.clear();

    line += fs(" %04zx   ", row);

    size_t n = std::min(bufferlen, ssize_t(16));

    for (size_t i = 0; i < n; i++) {
      if (i == 8) { line += " "; }
      line += fs(" %02x", data[i]);
    }
    for (size_t i = n; i < 16; i++) { line += "   "; }
    line += "   ";
    for (size_t i = 0; i < n; i++) {
      if (i == 8) { line += "  "; }
      uint8_t c = data[i];
      if (!(0x20 <= c && c <= 0x7e)) c = '.';
      line += fs("%c", c);
    }
    binarys.push_back(line);
    bufferlen -= n;
    data += n;
    row  += n;
  }
}



/*
 * Class Member Function Implementation
 */
std::string Packet::to_str() const
{
  using namespace slankdev;
  char sstr[1000];
#if __APPLE__
  sprintf(sstr, "%5zd %-13llu %-20s %-20s %-6s %5zd %-10s" , number, time,
#else
  sprintf(sstr, "%5zd %-13lu %-20s %-20s %-6s %5zd %-10s" , number, time,
#endif
          src.c_str(),
          dst.c_str(),
          protocol.c_str(), len,
          summary.c_str());
  return sstr;
}


void PacketListPane::key_input(int c)
{
  if (c == 'j' || c == KEY_DOWN) {
    cursor_down();
  } else if (c == 'k' || c == KEY_UP) {
    cursor_up();
  }
}

PacketListPane::PacketListPane(size_t _x, size_t _y, size_t _w, size_t _h)
  : PaneInterface(_x, _y, _w, _h), cursor(0), start_idx(0) {}


void PacketListPane::cursor_down()
{
  if (cursor + 1 < packets.size()) {
    if (cursor - start_idx + 2 > h) scroll_down();
    cursor++;
  }
}
void PacketListPane::cursor_up()
{
  if (cursor > 0) {
    if (cursor - 1 < start_idx) scroll_up();
    cursor--;
  }
}


