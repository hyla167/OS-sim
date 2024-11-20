//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef LRU
/* update_LRU_lst - update LRU list when there are reference 
 * (e.g: read, write) to existing page in that list
 * @pte: page table entry
 *
 */
int update_LRU_lst(uint32_t *pte)
{
#ifdef SYNC
  pthread_mutex_lock(&LRU_lock);
#endif
  struct LRU_list *p = lru_head;
  int fpn = PAGING_PTE_FPN(*pte);
  while (p->lru_next != NULL) {
    if (p->fpn == fpn) {
      break;
    }
    p = p->lru_next;
  }
  if (p == lru_head) {
    if (lru_head == lru_tail) {
#ifdef SYNC
      pthread_mutex_unlock(&LRU_lock);
#endif
      return 0;
    }
    lru_head = lru_head->lru_next;
    lru_head->lru_pre = NULL;
  }
  else if (p == lru_tail) {
#ifdef SYNC
    pthread_mutex_unlock(&LRU_lock);
#endif
    return 0;
  }
  else {
    p->lru_pre->lru_next = p->lru_next;
    p->lru_next->lru_pre = p->lru_pre;
  }
  lru_tail->lru_next = p;
  p->lru_pre = lru_tail;
  p->lru_next = NULL;
  lru_tail = p;
#ifdef SYNC
  pthread_mutex_unlock(&LRU_lock);
#endif
  return 0;
}

/* add_LRU_page - add newly allocated page to LRU list 
 * @pte: page table entry
 * @pgn: allocated page number in virtual memory
 */
int add_LRU_page(uint32_t *pte, int pgn)
{
#ifdef SYNC
  pthread_mutex_lock(&LRU_lock);
#endif
  struct LRU_list *tmp = malloc(sizeof(struct LRU_list));
  tmp->pte = pte;
  tmp->fpn = PAGING_PTE_FPN(*pte);
  tmp->pgn = pgn;
  if (lru_head == NULL)
  {
    lru_head = tmp;
    lru_tail = lru_head;
    tmp->lru_pre = NULL;
    tmp->lru_next = NULL;
  }
  else
  {
    struct LRU_list *p = lru_head;
    int flag = 0;
    while (p != NULL)
    {
      // check if fpn of pte are coincided
      if (p->fpn == tmp->fpn)
      {
        flag = 1;
        break;
      }
      p = p->lru_next;
    }
    if (flag == 1)
    {
      if (p == lru_head)
      {
        if (lru_head == lru_tail)
        {
#ifdef SYNC
          pthread_mutex_unlock(&LRU_lock);
#endif
          return 0;
        }
        lru_head = lru_head->lru_next;
        lru_head->lru_pre = NULL;
      }
      else if (p == lru_tail)
      {
#ifdef SYNC
        pthread_mutex_unlock(&LRU_lock);
#endif
        return 0;
      }
      else {
        p->lru_pre->lru_next = p->lru_next;
        p->lru_next->lru_pre = p->lru_pre;
      }
      lru_tail->lru_next = p;
      p->lru_pre = lru_tail;
      p->lru_next = NULL;
      lru_tail = p;
    }
    else {
      lru_tail->lru_next = tmp;
      tmp->lru_pre = lru_tail;
      tmp->lru_next = NULL;
      lru_tail = tmp;
#ifdef SYNC
      pthread_mutex_unlock(&LRU_lock);
#endif
      return 0;
    }
  }
#ifdef SYNC
  pthread_mutex_unlock(&LRU_lock);
#endif
  return 0;
}

/* find_LRU_victim_page - find next page to swap out 
 * with LRU replacement policy
 * @pgn: returned page number
 * @fpn: returned frame number
 * @pte: returned page table entry
 */
int find_LRU_victim_page(int* pgn, int* fpn, uint32_t** pte) {
#ifdef SYNC
  pthread_mutex_lock(&LRU_lock);
#endif
  if (lru_head == NULL) {
#ifdef SYNC
    pthread_mutex_unlock(&LRU_lock);
#endif
    return -1;
  }
  struct LRU_list *temp = lru_head;
  *pgn = temp->pgn; // get victim pgn
  *fpn = temp->fpn; // get victim fpn
  *pte = temp->pte;
  if (lru_head == lru_tail) {
    lru_head = lru_tail = NULL;
  }
  else {
    lru_head = lru_head->lru_next;

    if (lru_head != NULL) {
      lru_head->lru_pre = NULL;
    }
    temp->lru_next = NULL;
  }
  // free(temp);
#ifdef SYNC
  pthread_mutex_unlock(&LRU_lock);
#endif
  return 0;
}

/* print_LRU_page - print LRU list for debugging
 */
int print_LRU_page() {
#ifdef SYNC
  pthread_mutex_lock(&LRU_lock);
#endif
  struct LRU_list *temp = lru_head;
  printf(ANSI_COLOR_TEAL "-----------------LRU LIST-----------------\n");
  if (temp == NULL) {
    printf("\nEMPTY page directory\n");
    printf("\n------------------------------------------\n\n" ANSI_COLOR_RESET);
  }
  else {
    while (temp != NULL) {
      // printf("[%d]", temp->fpn);
      printf("[%d]", temp->pgn);
      if (temp->lru_next != NULL) {
        printf(" -> ");
      }
      temp = temp->lru_next;
    }
    printf("\n------------------------------------------\n\n" ANSI_COLOR_RESET);
  }
#ifdef SYNC
  pthread_mutex_unlock(&LRU_lock);
#endif
  return 0;
}
#endif

// added param vmaid, vm_rg_struct* rg_elmt
int enlist_vm_freerg_list(struct mm_struct *mm, int vmaid, struct vm_rg_struct* rg_elmt) {
  //struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list; 
  struct vm_area_struct* cur_vma = get_vma_by_num(mm, vmaid);
  struct vm_rg_struct* rg_node = cur_vma->vm_freerg_list;

  if (((rg_elmt->rg_start >= rg_elmt->rg_end) && (vmaid == 0)) ||
      ((rg_elmt->rg_start <= rg_elmt->rg_end) && (vmaid == 1))) 
    return -1;

  if (rg_node != NULL)
    rg_elmt->rg_next = rg_node; 

  cur_vma->vm_freerg_list = rg_elmt;

  return 0;
}

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma= mm->mmap;

  if(mm->mmap == NULL)
    return NULL;

  int vmait = 0;
  
  while (vmait < vmaid)
  {
    if(pvma == NULL)
	  return NULL;

    vmait++;
    pvma = pvma->vm_next;
  }

  return pvma;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  #ifdef LRU
#ifdef SYNC
  pthread_mutex_lock(&MEM_in_use);
#endif
  uint32_t pte = mm->pgd[pgn];
  if (!PAGING_PTE_PAGE_PRESENT(pte)) { 
    int fpn_temp = -1;
    if (MEMPHY_get_freefp(caller->mram, &fpn_temp) == 0) {
      int tgtfpn = PAGING_PTE_FPN(pte);
      __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, fpn_temp);
      pte_set_fpn(&mm->pgd[pgn], fpn_temp);
    }
    else {
      int tgtfpn = PAGING_PTE_FPN(pte);
      int vicfpn, swpfpn, vicpgn;
      uint32_t* vicpte;
      // get victim page 
      find_LRU_victim_page(&vicpgn, &vicfpn, &vicpte);
      if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) < 0) {
        printf("Out of SWAP");
#ifdef SYNC
        pthread_mutex_unlock(&MEM_in_use);
#endif
        return -3000;
      }
#ifdef RAM_STATUS_DUMP
      printf(ANSI_COLOR_PINK "[Page Replacement]\tPID #%d:\tVic FPN:%d\tVic PGN:%d\tPTE:%08x\tTarget:%d\t\n" ANSI_COLOR_RESET, caller->pid, vicfpn, vicpgn, *vicpte, tgtfpn);
#endif
      // Copy victim frame to swap
      __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
      // Copy target frame from swap to mem 
      __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);
      /* Find index of avtive memswap */
      int index_of_active_mswp;
      for (index_of_active_mswp = 0; index_of_active_mswp < PAGING_MAX_MMSWP; index_of_active_mswp++) {
        if ((struct memphy_struct *)&caller->mswp[index_of_active_mswp] == caller->active_mswp) break;
      }
      printf("Debug: vicfpn: %d, swpfpn: %d, tgtfpn: %d\n, index_active: %d\n", vicfpn, swpfpn, tgtfpn, index_of_active_mswp);
      pte_set_swap(vicpte, index_of_active_mswp, swpfpn);
      // signal pte has new fpn
      pte_set_fpn(&mm->pgd[pgn], vicfpn);

#ifdef RAM_STATUS_DUMP
      printf(ANSI_COLOR_PINK "[After Swap]\tPID #%d:\tVic FPN:%d\tVic PGN:%d\tPTE:%08x\tTarget:%d\t\n" ANSI_COLOR_RESET, caller->pid, vicfpn, vicpgn, *vicpte, tgtfpn);
#endif
      MEMPHY_put_freefp(caller->active_mswp, tgtfpn);
    }
  }
  add_LRU_page(&mm->pgd[pgn], pgn);
  // *fpn = GETVAL(mm->pgd[pgn], PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
  *fpn = PAGING_PTE_FPN(mm->pgd[pgn]);
#ifdef SYNC
  pthread_mutex_unlock(&MEM_in_use);
#endif
  #else
  #ifdef SYNC
    pthread_mutex_lock(&MEM_in_use);
  #endif
  uint32_t pte = mm->pgd[pgn];
 
  if (!PAGING_PTE_PAGE_PRESENT(pte))
  { /* Page is not online, make it actively living */
    printf("\tPage is not online, make it actively living\n");
    int vicfpn;

    // int tgtfpn = PAGING_SWP(pte);//the target frame storing our variable ~ bit 8 -> bit 23 * 8
    int swpoff = PAGING_SWPOFF(pte); // Frame storing our variable ~ bit 5 -> bit 25
    int swptyp = PAGING_SWPTYP(pte); // Index of memswap 


    /* TODO: Play with your paging theory here */
    if (MEMPHY_get_freefp(caller->mram, &vicfpn) < 0) // RAM doesn't have any free frame -> Paging
    {
      printf("\tRAM doesn't have any free frame -> Paging\n");
      int vicpgn, swpfpn; 
      uint32_t vicpte;
      /* Find victim page */
      if (find_victim_page(mm, &vicpgn) < 0) return -1;

      vicpte = mm->pgd[vicpgn];
      vicfpn = PAGING_PTE_FPN(vicpte);
      printf("\tVictim pgn: %d - Victim fpn: %d\n", vicpgn, vicfpn);
      /* Find index of active memswap */
      int index_of_active_mswp;
      for (index_of_active_mswp = 0; index_of_active_mswp < PAGING_MAX_MMSWP; index_of_active_mswp++) {
        if ((struct memphy_struct *)&caller->mswp[index_of_active_mswp] == caller->active_mswp) break;
      }

      /* Get free frame in MEMSWP */
      if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) < 0) {
        printf("Current active swap doesn't have any free frame\n"); fflush(stdout);
        int i;
        for (i = 0; i < PAGING_MAX_MMSWP; i++) {
          if (MEMPHY_get_freefp(caller->mswp[i], &swpfpn) == 0) {
            caller->active_mswp = (struct memphy_struct *)&caller->mswp[i];
            index_of_active_mswp = i;
            break;
          }
        }
        if (i == PAGING_MAX_MMSWP) {
          #ifdef SYNC
            pthread_mutex_unlock(&MEM_in_use);
          #endif
          return -3000; // MEMSWAP doesn't have any free frame
        }
      }

      /* Copy victim frame to swap */
      printf("\tCopy fpn=%d to swpfpn=%d\n", vicfpn, swpfpn); fflush(stdout);
      __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
      /* Update page table */
      pte_set_swap(&mm->pgd[vicpgn], index_of_active_mswp, swpfpn);
    }
    /* Do swap frame from MEMRAM to MEMSWP and vice versa*/
    /* Copy target frame from swap to mem */
    __swap_cp_page((struct memphy_struct *)&caller->mswp[swptyp], swpoff, caller->mram, vicfpn);
    MEMPHY_put_freefp((struct memphy_struct *)&caller->mswp[swptyp], swpoff);

    pte_set_fpn(&pte, vicfpn);
    pte_set_fpn(&mm->pgd[pgn], vicfpn);
    enlist_pgn_node(&caller->mm->fifo_pgn,pgn);
  }

  *fpn = PAGING_PTE_FPN(pte);
  #ifdef SYNC
    pthread_mutex_unlock(&MEM_in_use);
  #endif
  #endif
  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess 
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller, int vmaid)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn, phyaddr;
  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if(pg_getpage(mm, pgn, &fpn, caller) != 0) 
    return -1; /* invalid page access */
  if (vmaid == 0) {
    phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;
  }
  else { // vmaid = 1
    phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + (PAGING_PAGESZ - 1 - off);
  }
  MEMPHY_write(caller->mram,phyaddr, value);
  return 0;
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
#ifdef RAM_STATUS_DUMP
  printf(ANSI_COLOR_PINK "------------------------------------------\n");
  if (vmaid == 0) {
    printf("Process %d ALLOC CALL | SIZE = %d\n" ANSI_COLOR_RESET, caller->pid, size);
  }
  else { // vmaid = 1
    printf("Process %d MALLOC CALL | SIZE = %d\n" ANSI_COLOR_RESET, caller->pid, size);
  }
#endif
  /*Allocate at the toproof */
  struct vm_rg_struct rgnode;

  /* TODO: commit the vmaid */
  rgnode.vmaid = vmaid;

  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;

    caller->mm->symrgtbl[rgid].vmaid = rgnode.vmaid;

    *alloc_addr = rgnode.rg_start;
    #ifdef LRU
    if (vmaid == 0) {
      for (int i = rgnode.rg_start / 256; i <= rgnode.rg_end / 256; i++)
      {
        uint32_t current_pte = caller->mm->pgd[i];
        if (!PAGING_PTE_PAGE_PRESENT(current_pte))
        {
          printf("FREE REGION in SWAP \n\n");
          int tgtfpn = PAGING_PTE_FPN(current_pte);
          uint32_t* vicpte;
          int vicfpn, swpfpn, vicpgn;

          find_LRU_victim_page(&vicpgn, &vicfpn, &vicpte);
          if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) < 0)
          {
            printf("Out of SWAP");
            return -3000;
          }
          __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
          /* Find index of active memswap */
          int index_of_active_mswp;
          for (index_of_active_mswp = 0; index_of_active_mswp < PAGING_MAX_MMSWP; index_of_active_mswp++) {
            if ((struct memphy_struct *)&caller->mswp[index_of_active_mswp] == caller->active_mswp) break;
          }
          pte_set_swap(vicpte, index_of_active_mswp, swpfpn);
          __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);
          pte_set_fpn(&caller->mm->pgd[i], vicfpn);

          add_LRU_page(&caller->mm->pgd[i], i);
          MEMPHY_put_freefp(caller->active_mswp, tgtfpn);
        }
        update_LRU_lst(&current_pte);
      }
    }
    else {
      for (int i = rgnode.rg_start / 256; i >= rgnode.rg_end / 256; i--)
      {
        uint32_t current_pte = caller->mm->pgd[i];
        if (!PAGING_PTE_PAGE_PRESENT(current_pte))
        {
          printf("FREE REGION in SWAP \n\n");
          int tgtfpn = PAGING_PTE_FPN(current_pte);
          uint32_t* vicpte;
          int vicfpn, swpfpn, vicpgn;

          find_LRU_victim_page(&vicpgn, &vicfpn, &vicpte);

          if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) < 0)
          {
            printf("Out of SWAP");
            return -3000;
          }
          __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
          /* Find index of avtive memswap */
          int index_of_active_mswp;
          for (index_of_active_mswp = 0; index_of_active_mswp < PAGING_MAX_MMSWP; index_of_active_mswp++) {
            if ((struct memphy_struct *)&caller->mswp[index_of_active_mswp] == caller->active_mswp) break;
          }
          pte_set_swap(vicpte, index_of_active_mswp, swpfpn);
          __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);
          pte_set_fpn(&caller->mm->pgd[i], vicfpn);

          add_LRU_page(&caller->mm->pgd[i], i);
          MEMPHY_put_freefp(caller->active_mswp, tgtfpn);
        }
        update_LRU_lst(&current_pte);
      }
    }
    #endif

    #ifdef RAM_STATUS_DUMP
    printf(ANSI_COLOR_CYAN "FOUND A FREE region to alloc.\n");
    printf("------------------------------------------\n");
    printf("Process %d Allocated Region list \n", caller->pid);
    for (int it = 0; it < PAGING_MAX_SYMTBL_SZ; it++)
    {
      if (caller->mm->symrgtbl[it].rg_start == 0 && caller->mm->symrgtbl[it].rg_end == 0)
        continue;
      else
        printf("Region id %d : Range = [%lu - %lu] - vmaid = %d\n", it, caller->mm->symrgtbl[it].rg_start, caller->mm->symrgtbl[it].rg_end, caller->mm->symrgtbl[it].vmaid);
    }

    printf("------------------------------------------\n");
    printf("Process %d Free Region list \n", caller->pid);
    struct vm_area_struct *vma = caller->mm->mmap;
    while (vma != NULL) {
      struct vm_rg_struct *temp = vma->vm_freerg_list;
      while (temp != NULL)
      {
        if (temp->rg_start != temp->rg_end) {
          if (temp->vmaid == 0)
            printf("Range = [%lu - %lu], vmaid = 0, freespace = %lu\n", temp->rg_start, temp->rg_end, temp->rg_end - temp->rg_start + 1);
          else // vmaid = 1
            printf("Range = [%lu - %lu], vmaid = 1, freespace = %lu\n", temp->rg_start, temp->rg_end, temp->rg_start - temp->rg_end + 1);
        }
        temp = temp->rg_next;
      }
      vma = vma->vm_next;
    }
    printf("------------------------------------------\n");

    struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
    printf("VMA id %ld : start = %lu, end = %lu, sbrk = %lu\n" ANSI_COLOR_RESET, cur_vma->vm_id, cur_vma->vm_start, cur_vma->vm_end, cur_vma->sbrk);
    RAM_dump(caller->mram);
    #ifdef LRU
      print_LRU_page();
    #else
      print_list_pgn(caller->mm->fifo_pgn);
    #endif
    #endif
    return 0;
  }

  /* TODO: get_free_vmrg_area FAILED handle the region management (Fig.6)*/

  /* TODO retrive current vma if needed, current comment out due to compiler redundant warning*/
  /*Attempt to increate limit to get space */
  
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  
  /* TODO retrive old_sbrk if needed, current comment out due to compiler redundant warning*/
  int old_sbrk = cur_vma->sbrk;
  int inc_limit_ret = 0;
  int inc_sz = PAGING_PAGE_ALIGNSZ(size);
  /* TODO INCREASE THE LIMIT */
  if (vmaid == 0) {
    // Check for OOM
    #ifdef MM_PAGING_HEAP_GODOWN
    if (old_sbrk + size > caller->vmemsz) {
      printf(ANSI_COLOR_RED "ERROR: Out of virtual memory. Allocated Failed!\n" ANSI_COLOR_RESET);
      return -1;
    }
    #endif
    if (old_sbrk + size > cur_vma->vm_end) {
      // int inc_sz = cur_vma->sbrk + size - cur_vma->vm_end;
      inc_vma_limit(caller, vmaid, size, &inc_limit_ret);
      if (inc_limit_ret < 0) {
        return -1;
      }
    }
  }
  else { // vmaid = 1
    // Check for OOM
    if (old_sbrk - size < 0) {
      printf(ANSI_COLOR_RED "ERROR: Out of virtual memory. Allocated Failed!\n" ANSI_COLOR_RESET);
      return -1;
    }
    if (old_sbrk - size < (int)cur_vma->vm_end) {
      // int inc_sz = cur_vma->sbrk + size - cur_vma->vm_end;
      inc_vma_limit(caller, vmaid, size, &inc_limit_ret);
      if (inc_limit_ret < 0) {
        return -1;
      }
    }
  }
  // inc_vma_limit(caller, vmaid, inc_sz, &inc_limit_ret);

  /* TODO: commit the limit increment */

  /* TODO: commit the allocation address 
  // *alloc_addr = ...
  */

  if (vmaid == 0) {
    caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
    caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size - 1;
    cur_vma->sbrk = old_sbrk + size;
  }
  else { // vmaid = 1
  #ifdef MM_PAGING_HEAP_GODOWN
    if (old_sbrk == caller->vmemsz - 1) {
      caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
      caller->mm->symrgtbl[rgid].rg_end = old_sbrk + 1 - size;
      cur_vma->sbrk = old_sbrk + 1 - size;
    }
    else {
      caller->mm->symrgtbl[rgid].rg_start = old_sbrk - 1;
      caller->mm->symrgtbl[rgid].rg_end = old_sbrk - size;
      cur_vma->sbrk = old_sbrk - size;
    }
  #endif
  }
  caller->mm->symrgtbl[rgid].vmaid = vmaid;
  *alloc_addr = old_sbrk;
  #ifdef RAM_STATUS_DUMP
  printf(ANSI_COLOR_CYAN ">>>>>Done>>>>> #RGID: %d\n", rgid);
  printf("------------------------------------------\n" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_CYAN "Process %d Allocated Region list \n", caller->pid);
  for (int it = 0; it < PAGING_MAX_SYMTBL_SZ; it++)
  {
    if (caller->mm->symrgtbl[it].rg_start == 0 && caller->mm->symrgtbl[it].rg_end == 0)
      continue;
    else
      printf("Region id %d : Range = [%lu - %lu] - vmaid = %d\n", it, caller->mm->symrgtbl[it].rg_start, caller->mm->symrgtbl[it].rg_end, caller->mm->symrgtbl[it].vmaid);
  }

  printf("------------------------------------------\n");
  printf("Process %d Free Region list \n", caller->pid);
  // struct vm_rg_struct *temp = caller->mm->mmap->vm_freerg_list;
  struct vm_area_struct *vma = caller->mm->mmap;
  while (vma != NULL) {
    struct vm_rg_struct *temp = vma->vm_freerg_list;
    while (temp != NULL)
    {
      if (temp->rg_start != temp->rg_end) {
        if (temp->vmaid == 0)
          printf("Range = [%lu - %lu], vmaid = 0, freespace = %lu\n", temp->rg_start, temp->rg_end, temp->rg_end - temp->rg_start + 1);
        else // vmaid = 1
          printf("Range = [%lu - %lu], vmaid = 1, freespace = %lu\n", temp->rg_start, temp->rg_end, temp->rg_start - temp->rg_end + 1);
      }
      temp = temp->rg_next;
    }
    vma = vma->vm_next;
  }
  printf("------------------------------------------\n");

  printf("VMA id %ld : start = %lu, end = %lu, sbrk = %lu\n" ANSI_COLOR_RESET, cur_vma->vm_id, cur_vma->vm_start, cur_vma->vm_end, cur_vma->sbrk);
  RAM_dump(caller->mram);
    #ifdef LRU
    print_LRU_page();
    #else
    print_list_pgn(caller->mm->fifo_pgn);
    #endif
  #endif
  return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __free(struct pcb_t *caller, int rgid)
{
  struct vm_rg_struct* freergnode = malloc(sizeof(struct vm_rg_struct));

  struct vm_rg_struct* rgnode;

  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return -1;
  // Free dynamic memory for rgnode.vmaid = 0:
  /* TODO: Manage the collect freed region to freerg_list */
  rgnode = get_symrg_byid(caller->mm, rgid);
  int vmaid = rgnode->vmaid;
  if (rgnode->rg_start == rgnode->rg_end) {
    // This can happen e.g when process contains invalid instruction to free an unallocated memory region
    printf(ANSI_COLOR_RED "\tFree invalid range\n" ANSI_COLOR_RESET);
    return -1;
  }

  // deep copy rgnode to freergnode
  freergnode->rg_start = rgnode->rg_start;
  freergnode->rg_end = rgnode->rg_end;
  freergnode->vmaid = vmaid;
  freergnode->rg_next = NULL;

  // set all byte memory NULL
  for (unsigned long addr = rgnode->rg_start; addr < rgnode->rg_end; ++addr)
    pg_setval(caller->mm, addr, '\0', caller, vmaid);

  // remove the freed region from symbol table
  rgnode->rg_start = 0;
  rgnode->rg_end = 0;
  rgnode->rg_next = NULL;

  /*enlist the obsoleted memory region */
  enlist_vm_freerg_list(caller->mm, vmaid, freergnode); 
  #ifdef RAM_STATUS_DUMP
  printf(ANSI_COLOR_PINK "------------------------------------------\n");
  printf("Process %d FREE CALLED | Region id %d after free: [%lu,%lu] in vmaid %d\n" ANSI_COLOR_CYAN, caller->pid, rgid, rgnode->rg_start, rgnode->rg_end, vmaid);
  
  for (int it = 0; it < PAGING_MAX_SYMTBL_SZ; it++)
  {
    if (caller->mm->symrgtbl[it].rg_start == 0 && caller->mm->symrgtbl[it].rg_end == 0)
      continue;
    else
      printf("Region id %d : Range = [%lu - %lu] - vmaid = %d\n", it, caller->mm->symrgtbl[it].rg_start, caller->mm->symrgtbl[it].rg_end, caller->mm->symrgtbl[it].vmaid);
  }
  printf("------------------------------------------\n");
  printf("Process %d Free Region list \n", caller->pid);
  // struct vm_rg_struct *temp = caller->mm->mmap->vm_freerg_list;
  struct vm_area_struct *vma = caller->mm->mmap;
  while (vma != NULL) {
    struct vm_rg_struct *temp = vma->vm_freerg_list;
    while (temp != NULL)
    {
      if (temp->rg_start != temp->rg_end) {
        if (temp->vmaid == 0)
          printf("Range = [%lu - %lu], vmaid = 0, freespace = %lu\n", temp->rg_start, temp->rg_end, temp->rg_end - temp->rg_start + 1);
        else // vmaid = 1
          printf("Range = [%lu - %lu], vmaid = 1, freespace = %lu\n", temp->rg_start, temp->rg_end, temp->rg_start - temp->rg_end + 1);
      }
      temp = temp->rg_next;
    }
    vma = vma->vm_next;
  }
  printf("------------------------------------------\n" ANSI_COLOR_RESET);
  #endif
  return 0;

  // TODO: Free dynamic memory for rgnode.vmaid = 1?
}

/*pgalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;
  int val = __alloc(proc, 0, reg_index, size, &addr);
  if (val < 0) {
    printf(ANSI_COLOR_RED "\tAlloc FAILED\n" ANSI_COLOR_RESET);
  }
  return val;
}

/*pgmalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify vaiable in symbole table)
 */
int pgmalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;
  int val = __alloc(proc, 1, reg_index, size, &addr);
  if (val < 0) {
    printf(ANSI_COLOR_RED "\tAlloc FAILED\n" ANSI_COLOR_RESET);
    return val;
  }
  return val;
}

/*pgfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int pgfree_data(struct pcb_t *proc, uint32_t reg_index)
{
  return __free(proc, reg_index);
}


/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess 
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller, int vmaid)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if(pg_getpage(mm, pgn, &fpn, caller) != 0) 
    return -1; /* invalid page access */

  int phyaddr = (vmaid == 0) ? (fpn << PAGING_ADDR_FPN_LOBIT) + off : (fpn << PAGING_ADDR_FPN_LOBIT) + (PAGING_PAGESZ - off - 1);

  MEMPHY_read(caller->mram,phyaddr, data);

  return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region 
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __read(struct pcb_t *caller, int rgid, int offset, BYTE *data)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  int vmaid = currg->vmaid;

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if(currg == NULL || cur_vma == NULL) /* Invalid memory identify */ {
	  return -1;
  }

  if (currg->rg_start == currg->rg_end) {
    printf(ANSI_COLOR_RED "\tRead invalid range" ANSI_COLOR_RESET "\n"); fflush(stdout);
    return -1;
  }
  if (vmaid == 0) {
    if (currg->rg_start + offset > currg->rg_end) {
      return -1; // offset out of region range
    }
  }
  else {
    if (currg->rg_start - offset < currg->rg_end) {
      return -1; // offset out of region range
    }
  }
  /* Update LRU list for each reference (i.e read, write) */
  if (vmaid == 0) {
    pg_getval(caller->mm, currg->rg_start + offset, data, caller, vmaid);
    #ifdef LRU
    update_LRU_lst(&caller->mm->pgd[PAGING_PGN((currg->rg_start + offset))]);
    #endif
  }
  else { // vmaid = 1
    pg_getval(caller->mm, currg->rg_start - offset, data, caller, vmaid);
    #ifdef LRU
    update_LRU_lst(&caller->mm->pgd[PAGING_PGN((currg->rg_start - offset))]);
    #endif
  }
  return 0;
}


/*pgwrite - PAGING-based read a region memory */
int pgread(
		struct pcb_t * proc, // Process executing the instruction
		uint32_t source, // Index of source register
		uint32_t offset, // Source address = [source] + [offset]
		uint32_t destination) 
{
  BYTE data;

  int val = __read(proc, source, offset, &data);
  destination = (uint32_t) data;
  #ifdef IODUMP
  if (val == 0)
    printf(ANSI_COLOR_PINK "Process %d read region=%d offset=%d value=%d\n" ANSI_COLOR_RESET, proc->pid, source, offset, data);
  else
    printf(ANSI_COLOR_RED "Process %d error when read region=%d offset=%d" ANSI_COLOR_RESET "\n", proc->pid, source, offset);
  #ifdef PAGETBL_DUMP
    print_pgtbl(proc, 0, -1); //print max TBL
  #endif
  #endif
  #ifdef RAM_STATUS_DUMP
  #ifdef LRU
  print_LRU_page();
  #else
  print_list_pgn(proc->mm->fifo_pgn);
  #endif
  #endif
  return val;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region 
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __write(struct pcb_t *caller, int rgid, int offset, BYTE value)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  int vmaid = currg->vmaid;

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  
  if(currg == NULL || cur_vma == NULL) /* Invalid memory identify */
	  return -1;

  if (currg->rg_start == currg->rg_end) {
    printf("\tWrite invalid range\n");
    return -1;
  }
  if (vmaid == 0) {
    if (currg->rg_start + offset > currg->rg_end) {
      return -1; // offset out of region range
    }
  }
  else { // vmaid = 1
    if (currg->rg_start - offset < currg->rg_end) {
      return -1; // offset out of region range
    }
  }
  /* Update LRU list for each reference (i.e read, write) */
  if (vmaid == 0) {
    pg_setval(caller->mm, currg->rg_start + offset, value, caller, 0);
    #ifdef LRU
    update_LRU_lst(&caller->mm->pgd[PAGING_PGN((currg->rg_start + offset))]);
    #endif
  }
  else { // vmaid = 1
    pg_setval(caller->mm, currg->rg_start - offset, value, caller, 1);
    #ifdef LRU
    update_LRU_lst(&caller->mm->pgd[PAGING_PGN((currg->rg_start - offset))]);
    #endif
  }
  return 0;
}

/*pgwrite - PAGING-based write a region memory */
int pgwrite(
		struct pcb_t * proc, // Process executing the instruction
		BYTE data, // Data to be wrttien into memory
		uint32_t destination, // Index of destination register
		uint32_t offset)
{
  int val = __write(proc, destination, offset, data);
  #ifdef IODUMP
    printf(ANSI_COLOR_PINK "write region=%d offset=%d value=%d\n" ANSI_COLOR_RESET, destination, offset, data);
  #ifdef PAGETBL_DUMP
    print_pgtbl(proc, 0, -1); 
  #endif
  #endif
  #ifdef RAM_STATUS_DUMP
  #ifdef LRU
  print_LRU_page();
  #else
  print_list_pgn(proc->mm->fifo_pgn);
  #endif
  #endif
  return val;
}


/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller) {
  int pagenum, fpn;
  uint32_t pte;


  for(pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++) {
    pte= caller->mm->pgd[pagenum];

    if (!PAGING_PTE_PAGE_PRESENT(pte)) {
      fpn = PAGING_PTE_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } 
    else {
      int fpn = PAGING_PTE_SWP(pte);
      fpn = PAGING_PTE_SWPOFF(pte) / PAGING_PAGESZ;
      MEMPHY_put_freefp(caller->active_mswp, fpn);    
    }
  }

  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct* get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz) {
  struct vm_rg_struct * newrg;
  /* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));

  // TODO: update the newrg boundary
  newrg->rg_start = cur_vma->sbrk;
  if (vmaid == 0) {
    newrg->rg_end = newrg->rg_start + size;
  }
  else {
    newrg->rg_end = newrg->rg_start - size;
  }
  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  struct vm_area_struct *vma = caller->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */
  while (vma != NULL) {
    if (vmaid == vma->vm_id) {
      vma = vma->vm_next;
      continue;
    }
    if (vma->vm_start == vma->vm_end) {
      vma = vma->vm_next;
      continue; 
    } 
    if ((((int)vma->vm_start-vmastart)*(vmaend-(int)vma->vm_end)) >= 0) {
      if (vmaend != vma->vm_end) {     
        printf(ANSI_COLOR_RED "ERROR: Memory overlap detected!: ");
        printf("[%ld - %ld] and [%d - %d]\n\n" ANSI_COLOR_RESET, vma->vm_start, vma->vm_end, vmastart, vmaend);
        return -1;
      }
    }
    vma = vma->vm_next;
  }
  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size 
 *@inc_limit_ret: increment limit return
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz, int* inc_limit_ret)
{
  struct vm_rg_struct * newrg = malloc(sizeof(struct vm_rg_struct));
  newrg->vmaid = vmaid;
  // int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz); // Num bytes allocated
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  int inc_amt = 0;
  if (vmaid == 0) {
    inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz - (cur_vma->vm_end - cur_vma->sbrk));
  }
  else { // vmaid = 1
    inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz - (cur_vma->sbrk - cur_vma->vm_end));
  }
  int incnumpage =  inc_amt / PAGING_PAGESZ; // Num pages allocated
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  int old_end = cur_vma->vm_end;

  /*Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0) {
    *inc_limit_ret = -1;
    return -1; /*Overlap and failed allocation */
  }
  /* TODO: Obtain the new vm area based on vmaid */
  // cur_vma->vm_end += inc_sz;
  if (vmaid == 0) {
    cur_vma->vm_end = PAGING_PAGE_ALIGNSZ(cur_vma->sbrk + inc_sz);
  }
  else {
    cur_vma->vm_end = PAGING_PAGE_ALIGNSZ(cur_vma->sbrk - inc_sz) - PAGING_PAGESZ; // to avoid rounding up
  }
  // inc_limit_ret...

  if (vm_map_ram(caller, area->rg_start, area->rg_end, 
                    old_end, incnumpage , newrg) < 0) {
    if (vmaid == 0) cur_vma->vm_end -= inc_sz;
    else cur_vma->vm_end += inc_sz;
    *inc_limit_ret = -1;
    return -1; /* Map the memory to MEMRAM */
                    }
  *inc_limit_ret = 0;
  return 0;

}

/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, int *retpgn) 
{
  struct pgn_t *pg = mm->fifo_pgn;

  /* TODO: Implement the theorical mechanism to find the victim page */
  // Use FIFO replacement policy
  if (!pg)
        return -1; // not important, can return any number

  if (!pg->pg_next)
  {
      *retpgn = pg->pgn;
      mm->fifo_pgn = NULL;
      free(pg);
      return 0;
  }

  struct pgn_t *prev_Page;
  while (pg->pg_next)
  {
      prev_Page = pg;
      pg = pg->pg_next;
  }

  *retpgn = pg->pgn;
  prev_Page->pg_next = NULL;
  free(pg);
  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size 
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
    return -1;

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  while (rgit != NULL)
  {
    if (vmaid == 0) {
      if (rgit->rg_start + size - 1 <= rgit->rg_end)
      { /* Current region has enough space */
        newrg->rg_start = rgit->rg_start;
        newrg->rg_end = rgit->rg_start + size - 1;

        /* Update left space in chosen region */
        if (rgit->rg_start + size - 1 < rgit->rg_end)
        {
          rgit->rg_start = rgit->rg_start + size;
        }
        else
        { /*Use up all space, remove current node */
          /*Clone next rg node */
          struct vm_rg_struct *nextrg = rgit->rg_next;

          /*Cloning */
          if (nextrg != NULL)
          {
            rgit->rg_start = nextrg->rg_start;
            rgit->rg_end = nextrg->rg_end;

            rgit->rg_next = nextrg->rg_next;

            free(nextrg);
          }
          else
          { /*End of free list */
            rgit->rg_start = rgit->rg_end;	//dummy, size 0 region
            rgit->rg_next = NULL;
          }
        }
      }
      else
      {
        rgit = rgit->rg_next;	// Traverse next rg
      }
    }
    else { // vmaid = 1
      if (rgit->rg_start - size + 1 >= rgit->rg_end)
      { /* Current region has enough space */
        newrg->rg_start = rgit->rg_start;
        newrg->rg_end = rgit->rg_start - size + 1;

        /* Update left space in chosen region */
        if (rgit->rg_start - size + 1 > rgit->rg_end)
        {
          rgit->rg_start = rgit->rg_start - size;
        }
        else
        { /*Use up all space, remove current node */
          /*Clone next rg node */
          struct vm_rg_struct *nextrg = rgit->rg_next;

          /*Cloning */
          if (nextrg != NULL)
          {
            rgit->rg_start = nextrg->rg_start;
            rgit->rg_end = nextrg->rg_end;

            rgit->rg_next = nextrg->rg_next;

            free(nextrg);
          }
          else
          { /*End of free list */
            rgit->rg_start = rgit->rg_end;	//dummy, size 0 region
            rgit->rg_next = NULL;
          }
        }
      }
      else
      {
        rgit = rgit->rg_next;	// Traverse next rg
      }
    }
  }

 if(newrg->rg_start == -1) // new region not found
   return -1;

 return 0;
}

//#endif
