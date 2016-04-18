#include <kern/e1000.h>

// LAB 6: Your driver code here
#include <inc/assert.h>
#include <inc/error.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <kern/pmap.h>

struct tx_desc tx_desc_array[E1000_TXDESC] __attribute__ ((aligned (16)));
struct tx_pkt tx_pkt_bufs[E1000_TXDESC];

struct rcv_desc rcv_desc_array[E1000_RCVDESC] __attribute__ ((aligned (16)));
struct rcv_pkt rcv_pkt_bufs[E1000_RCVDESC];

// LAB 6: Your driver code here
int
e1000_pci_attach(struct pci_func *pcif)
{
  uint32_t i;

  // enable PCI device
  pci_func_enable(pcif);

  // create a virtual memory mapping for the E1000’s BAR 0
  e1000 = (uint32_t *)mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);

  // device status register should get 0x80080783
  assert(e1000[E1000_STATUS] == 0x80080783);

  // tx buffer array
  memset(tx_desc_array, 0, sizeof(struct tx_desc) * E1000_TXDESC);
  memset(tx_pkt_bufs, 0, sizeof(struct tx_pkt) * E1000_TXDESC);
  for (i = 0; i < E1000_TXDESC; i++) {
    tx_desc_array[i].addr = PADDR(tx_pkt_bufs[i].buf);
    tx_desc_array[i].status |= E1000_TXD_STAT_DD;
  }

  // Initialize rcv desc buffer array
  memset(rcv_desc_array, 0x0, sizeof(struct rcv_desc) * E1000_RCVDESC);
  memset(rcv_pkt_bufs, 0x0, sizeof(struct rcv_pkt) * E1000_RCVDESC);
  for (i = 0; i < E1000_RCVDESC; i++) {
    rcv_desc_array[i].addr = PADDR(rcv_pkt_bufs[i].buf);
  }

  // transmit descriptor
  e1000[E1000_TDBAL] = PADDR(tx_desc_array);
  e1000[E1000_TDBAH] = 0x0;
  e1000[E1000_TDLEN] = sizeof(struct tx_desc) * E1000_TXDESC;
  e1000[E1000_TDH] = 0x0;
  e1000[E1000_TDT] = 0x0;
// Initialize the Transmit Control Register 
  e1000[E1000_TCTL] |= E1000_TCTL_EN;
  e1000[E1000_TCTL] |= E1000_TCTL_PSP;
  e1000[E1000_TCTL] &= ~E1000_TCTL_CT;
  e1000[E1000_TCTL] |= (0x10) << 4;
  e1000[E1000_TCTL] &= ~E1000_TCTL_COLD;
  e1000[E1000_TCTL] |= (0x40) << 12;

  // Program the Transmit IPG Register
  e1000[E1000_TIPG] = 0x0;
  e1000[E1000_TIPG] |= (0x6) << 20; // IPGR2 
  e1000[E1000_TIPG] |= (0x4) << 10; // IPGR1
  e1000[E1000_TIPG] |= 0xA; // IPGR

  /* Receive Initialization */
  // Program the Receive Address Registers
  e1000[E1000_EERD] = 0x0;
  e1000[E1000_EERD] |= E1000_EERD_START;
  while (!(e1000[E1000_EERD] & E1000_EERD_DONE));
  e1000[E1000_RAL] = e1000[E1000_EERD] >> 16;

  e1000[E1000_EERD] = 0x1 << 8;
  e1000[E1000_EERD] |= E1000_EERD_START;
  while (!(e1000[E1000_EERD] & E1000_EERD_DONE));
  e1000[E1000_RAL] |= e1000[E1000_EERD] & 0xffff0000;
  cprintf("ral %p\n", e1000[E1000_RAL]);

  e1000[E1000_EERD] = 0x2 << 8;
  e1000[E1000_EERD] |= E1000_EERD_START;
  while (!(e1000[E1000_EERD] & E1000_EERD_DONE));
  e1000[E1000_RAH] = e1000[E1000_EERD] >> 16;
  cprintf("rah %p\n", e1000[E1000_RAH]);

  e1000[E1000_RAH] |= 0x1 << 31;
  //e1000[E1000_RAH] = 0x12005452;
  //e1000[E1000_RAL] = 0x5634 | (1 << 31);

  // Program the Receive Descriptor Base Address Registers
  e1000[E1000_RDBAL] = PADDR(rcv_desc_array);
  e1000[E1000_RDBAH] = 0x0;

  // Set the Receive Descriptor Length Register
  e1000[E1000_RDLEN] = sizeof(struct rcv_desc) * E1000_RCVDESC;

  // Set the Receive Descriptor Head and Tail Registers
  e1000[E1000_RDH] = 0x0;
  e1000[E1000_RDT] = E1000_RCVDESC - 1;
  //e1000[E1000_RDT] = 0x0; //FUCK!

  // Initialize the Receive Control Register
  e1000[E1000_RCTL] |= E1000_RCTL_EN;
  e1000[E1000_RCTL] &= ~E1000_RCTL_LPE;
  e1000[E1000_RCTL] &= ~E1000_RCTL_LBM;
  e1000[E1000_RCTL] &= ~E1000_RCTL_RDMTS;
  e1000[E1000_RCTL] &= ~E1000_RCTL_MO;
  e1000[E1000_RCTL] |= E1000_RCTL_BAM;
  e1000[E1000_RCTL] &= ~E1000_RCTL_SZ; // 2048 byte size
  e1000[E1000_RCTL] |= E1000_RCTL_SECRC;
  cprintf("e1000[RCTL]: %p\n", e1000[E1000_RCTL]);

  return 0;
}

#if defined(HAVE_DUMP_E1000_PACKET)
void
hexdump(const char *prefix, const void *data, int len)
{
    int i;
    char buf[80];
    char *end = buf + sizeof(buf);
    char *out = NULL;
    for (i = 0; i < len; i++) {
        if (i % 16 == 0)
            out = buf + snprintf(buf, end - buf,
                         "%s%04x   ", prefix, i);
        out += snprintf(out, end - out, "%02x", ((uint8_t*)data)[i]);
        if (i % 16 == 15 || i == len - 1)
            cprintf("%.*s\n", out - buf, buf);
        if (i % 2 == 1)
            *(out++) = ' ';
        if (i % 16 == 7)
            *(out++) = ' ';
    }
}
#define HEXDUMP(prefix, data, len) hexdump(prefix, data, len);
#else
#define HEXDUMP(prefix, data, len)
#endif

int
e1000_pkt_transmit(char *data, int length)
{
  if (length > TX_PKT_SIZE) {
    return -E_PKT_TOO_LONG;
  }

  //  transmit a packet by checking that the next descriptor is free
  uint32_t tdt = e1000[E1000_TDT];
  if (tx_desc_array[tdt].status & E1000_TXD_STAT_DD) {
    // copying the packet data into the next descriptor and updating TDT
    memmove(tx_pkt_bufs[tdt].buf, data, length);
    tx_desc_array[tdt].length = length;
    tx_desc_array[tdt].status &= ~E1000_TXD_STAT_DD;
    tx_desc_array[tdt].cmd |= E1000_TXD_CMD_RS;
    tx_desc_array[tdt].cmd |= E1000_TXD_CMD_EOP;
    e1000[E1000_TDT] = (tdt + 1) % E1000_TXDESC;
  } else { 
    // handle the transmit queue being full
    return -E_TX_FULL;
  }
  
  return 0;
}

int
e1000_pkt_receive(char *data) {
  uint32_t rdt, idx, len;
  rdt = e1000[E1000_RDT];
  idx = (rdt + 1) % E1000_RCVDESC;
  
  if (rcv_desc_array[idx].status & E1000_RXD_STAT_DD) {
    //cprintf("in pkt receive\n");
    if (!(rcv_desc_array[idx].status & E1000_RXD_STAT_EOP)) {
      panic("Don't allow jumbo frames!\n");
    }
    len = rcv_desc_array[idx].length;
    
    memmove(data, rcv_pkt_bufs[idx].buf, len);
    //cprintf("E1000 rx len: %0dx\n", len);
    HEXDUMP("rx dump:", data, len);
    rcv_desc_array[idx].status &= ~E1000_RXD_STAT_DD;
    rcv_desc_array[idx].status &= ~E1000_RXD_STAT_EOP;
    e1000[E1000_RDT] = idx;

    return len;
  }

  return -E_RCV_EMPTY;
}

