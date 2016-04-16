#include <kern/e1000.h>

// LAB 6: Your driver code here
#include <inc/assert.h>
#include <inc/error.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <kern/pmap.h>

struct tx_desc tx_desc_array[E1000_TXDESC] __attribute__ ((aligned (16)));
struct tx_pkt tx_pkt_bufs[E1000_TXDESC];

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

  // transmit descriptor
  e1000[E1000_TDBAL] = PADDR(tx_desc_array);
  e1000[E1000_TDBAH] = 0x0;
  e1000[E1000_TDLEN] = sizeof(struct tx_desc) * E1000_TXDESC;
  e1000[E1000_TDH] = 0x0;
  e1000[E1000_TDT] = 0x0;
  e1000[E1000_TCTL] = E1000_TCTL_EN | E1000_TCTL_PSP | (E1000_TCTL_CT & ((0x10) << 4)) | (E1000_TCTL_COLD & ((0x40) << 12));
  e1000[E1000_TIPG] = 0x0 | ((0x6) << 20) | ((0x4) << 10) | 0xA; 

  return 0;
}

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



