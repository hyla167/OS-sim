//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory physical module mm/mm-memphy.c
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[38;5;13m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_PINK  "\x1b[38;5;225m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/*
 *  MEMPHY_mv_csr - move MEMPHY cursor
 *  @mp: memphy struct
 *  @offset: offset
 */
int MEMPHY_mv_csr(struct memphy_struct *mp, int offset)
{
   pthread_mutex_lock(&mp->mutex);
   int numstep = 0;

   mp->cursor = 0;
   while(numstep < offset && numstep < mp->maxsz){
     /* Traverse sequentially */
     mp->cursor = (mp->cursor + 1) % mp->maxsz;
     numstep++;
   }
   pthread_mutex_unlock(&mp->mutex);
   return 0;
}

/*
 *  MEMPHY_seq_read - read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_seq_read(struct memphy_struct *mp, int addr, BYTE *value)
{
   if (mp == NULL) {
      pthread_mutex_unlock(&mp->mutex);
     return -1;
   }

   if (!mp->rdmflg) {
      pthread_mutex_unlock(&mp->mutex);
     return -1; /* Not compatible mode for sequential read */
   }
   MEMPHY_mv_csr(mp, addr);
   *value = (BYTE) mp->storage[addr];
   pthread_mutex_unlock(&mp->mutex);
   return 0;
}

/*
 *  MEMPHY_read read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
   if (mp == NULL)
     return -1;
   pthread_mutex_lock(&mp->mutex);
   if (mp->rdmflg)
      *value = mp->storage[addr];
   else /* Sequential access device */
      return MEMPHY_seq_read(mp, addr, value);
   pthread_mutex_unlock(&mp->mutex);
   return 0;
}

/*
 *  MEMPHY_seq_write - write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_seq_write(struct memphy_struct * mp, int addr, BYTE value)
{

   if (mp == NULL) {
      pthread_mutex_unlock(&mp->mutex);
     return -1;
   }
   if (!mp->rdmflg) {
      pthread_mutex_unlock(&mp->mutex);
     return -1; /* Not compatible mode for sequential read */
   }
   MEMPHY_mv_csr(mp, addr);
   mp->storage[addr] = value;
   pthread_mutex_unlock(&mp->mutex);
   return 0;
}

/*
 *  MEMPHY_write-write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
   if (mp == NULL)
     return -1;
   pthread_mutex_lock(&mp->mutex);
   if (mp->rdmflg)
      mp->storage[addr] = data;
   else /* Sequential access device */
      return MEMPHY_seq_write(mp, addr, data);
   pthread_mutex_unlock(&mp->mutex);
   return 0;
}

/*
 *  MEMPHY_format-format MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_format(struct memphy_struct *mp, int pagesz)
{
   pthread_mutex_lock(&mp->mutex);
    /* This setting come with fixed constant PAGESZ */
    int numfp = mp->maxsz / pagesz;
    struct framephy_struct *newfst, *fst;
    int iter = 0;

    if (numfp <= 0) {
      pthread_mutex_unlock(&mp->mutex);
      return -1;
    }

    /* Init head of free framephy list */ 
    fst = malloc(sizeof(struct framephy_struct));
    fst->fpn = iter;
    mp->free_fp_list = fst;

    /* We have list with first element, fill in the rest num-1 element member*/
    for (iter = 1; iter < numfp ; iter++)
    {
       newfst =  malloc(sizeof(struct framephy_struct));
       newfst->fpn = iter;
       newfst->fp_next = NULL;
       fst->fp_next = newfst;
       fst = newfst;
    }
   pthread_mutex_unlock(&mp->mutex);
    return 0;
}

int MEMPHY_get_freefp(struct memphy_struct *mp, int *retfpn)
{
   pthread_mutex_lock(&mp->mutex);
   struct framephy_struct *fp = mp->free_fp_list;

   if (fp == NULL) {
      pthread_mutex_unlock(&mp->mutex);
     return -1;
   }
   *retfpn = fp->fpn;
   mp->free_fp_list = fp->fp_next;
   /* MEMPHY is iteratively used up until its exhausted
    * No garbage collector acting then it not been released
    */
   free(fp);
   pthread_mutex_unlock(&mp->mutex);
   return 0;
}

int MEMPHY_dump(struct memphy_struct * mp)
{
    /*TODO dump memphy contnt mp->storage 
     *     for tracing the memory content
     */
#ifdef SYNC
  pthread_mutex_lock(&MEM_in_use);
#endif
   printf("\n");
   printf(ANSI_COLOR_GREEN "Print content of RAM (only print nonzero value)\n");
   for (int i = 0; i < mp->maxsz; i++)
   {
      if (mp->storage[i] != 0)
      {
         printf("---------------------------------\n");
         printf("Address 0x%08x: %d\n", i, mp->storage[i]);
      }
   }
   printf("---------------------------------\n" ANSI_COLOR_RESET);
#ifdef SYNC
  pthread_mutex_unlock(&MEM_in_use);
#endif
    return 0;
}

int RAM_dump(struct memphy_struct *mram)
{
  int freeCnt = 0;
  struct framephy_struct *fpit = mram->free_fp_list;
  while (fpit != NULL)
  {
    fpit = fpit->fp_next;
    freeCnt++;
  }
  printf(ANSI_COLOR_CYAN "----------- RAM mapping status -----------\n");
  printf("Number of mapped frames:\t%d\n", mram->maxsz / PAGING_PAGESZ - freeCnt);
  printf("Number of remaining frames:\t%d\n", freeCnt);
  printf("------------------------------------------\n" ANSI_COLOR_RESET);
  return 0;
}

int MEMPHY_put_freefp(struct memphy_struct *mp, int fpn)
{
   pthread_mutex_lock(&mp->mutex);
   struct framephy_struct *fp = mp->free_fp_list;
   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));

   /* Create new node with value fpn */
   newnode->fpn = fpn;
   newnode->fp_next = fp;
   mp->free_fp_list = newnode;
   pthread_mutex_unlock(&mp->mutex);
   return 0;
}


/*
 *  Init MEMPHY struct
 */
int init_memphy(struct memphy_struct *mp, int max_size, int randomflg)
{
   pthread_mutex_init(&mp->mutex, NULL);
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;

   MEMPHY_format(mp,PAGING_PAGESZ);

   mp->rdmflg = (randomflg != 0)?1:0;

   if (!mp->rdmflg )   /* Not Ramdom acess device, then it serial device*/
      mp->cursor = 0;
   
   return 0;
}

//#endif
