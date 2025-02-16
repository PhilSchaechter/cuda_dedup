# cuda_dedup
My attempt at a GPU-based deduplication algorithm + raw storage system

The storage system is multi-threaded, maintain an index of already found segments and storing each unique segment once.  It stores in a
raw filesystem device, in an append-only log format.   There are both cude and non-cuda versions of the system here, but I realized due
to restrictions in cuda and memory alignment, running it on a GPU is not more efficient than on the CPU (for now!) 

I do have an implementation here of a CUDA-based hashing algorithm which was much more successful performance wise.  This was my taking
an off-the-shelf algorithm and CUDA-fying it. 
