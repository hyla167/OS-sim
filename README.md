# Credit
The code (except for heap godown implementation) is partially taken from https://github.com/buithekys/os_assignmentHK232 with some modifications:

•	Add colored outputs

•	Modify LRU list output to show page number instead of frame number (for consistency with `print_list_pgn`)

•	Modify range of allocated region. For example, if process 1 allocated 200 bytes in stack, then the allocated region should be [0-199] instead of [0-200]

•	Optimize virtual memory allocation: In the original code, if P1 allocated 200 bytes in stack twice, then the allocated region would be [0-200] and [256-456]. This lead to a waste of VM in range [200-256]. Whereas in this implementation, the two allocated regions would be [0-199] and [200-399]

# Test cases
## Scheduler
`test_sched_slot`: test MLQ scheduling algorithm with priority-slot mechanism. For example, suppose that process P1 (`prio = 138 -> slot = 2`) and P2 (`prio = 137 -> slot = 3`) starts at the same time, time slice = 3. If P1 executed first, then after 2 time slots it must be put back in ready queue, although P1 still has 1 time slot left in its time slice.

`test_round-robin`: test round robin policy, i.e. all processes have the same priorities

`test_prior`: test priority scheduling, i.e. all processes have different priorites
## Memory Manangement
`test_mem`, `heap_0`: test basic function of OS: alloc, malloc, free, read, write, page replacement, write in invalid region, print page table directory, print RAM content, allocate to existing free regions, etc.

`test_mem2`: test page replacement policy when there are 2 different processes request to allocate memory at the same time.

`heap_1`: test OOM error due to heap over-allocation

`heap_2`: test OOM error due to stack over-allocation

`heap_3`: test OOM error due to stack-heap overlap
# Future improvements
1. **Optimize memory allocation**: In the current implementation, the size of vma is not reduced even when all of its allocated regions are freed. Further versions can modify this so that the stack/heap size is reduced when its top-most  page is freed (check `heap_4` for an example)
2. **Dirty bit**: Currently, modifying a page does not change its corresponding dirty bit in PTE. Further versions can implement this functionality to reduce page replacement time.
----
Note: Default page replacement policy is **FIFO**. To use **LRU** instead, make sure it is defined in `os-cfg.h`: `#define LRU`

