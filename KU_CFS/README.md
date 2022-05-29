# KU_CFS

### It aims to allocate fair CPU time to each process existing in the system by prioritizing through vruntime, which depends on the weight value multiplied by the execution time according to the nice value

<img src="https://github.com/alswlsghd320/KU_OS/blob/main/images/ku_cfs.png" width="640" height="480">


- Each task is stored in a ‘SINGLE’ ready queue.
- Executes in a single core system.
- The data structure of ready queue is linked list if possible (actually, red-black tree).
- In our CFS scheduler, nice value is from -2 to 2. 
- So, $vruntime = vruntime+DeltaExec*(1.25)^{nice value}$.
- Time slice and Executed time are 1 sec respectively.
