# KU_MMU

### It aims to manage physical memory in the arcitecture of multi-level page table.

<img src="https://github.com/alswlsghd320/KU_OS/blob/main/images/ku_mmu.png" width="640" height="480">

- Page replacement policy is FIFO.
- 8-bit addressing with 4 bytes page size. And each PTE/PDE is 1 byte;
- In PDE/PTE, the format is PFN[7:2]Unuse[1]Present[0] or Swapoffset[7:1]Present[0].
- In KU_MMU, We use 3-level page tables. <br/>
  (i.e. the format of va is PDIndex[7:6] PMDIndex[5:4] PTIndex[3:2] Offset [1:0])
- Only page frame for page(i.e. not page table) is swapped-out
