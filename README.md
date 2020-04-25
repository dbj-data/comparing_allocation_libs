# comparing_allocation_libs

> &copy; 2020 APR by dbj@dbj.org

### Quick (hopefully not questionable) comparison of few heap allocation libs

## Conclusion?

### System memory handling is still the most feasible to use.

Pool allocation for objects should be quicker. But. It is a narrow use case.
Also. Implementing object pool array allocator is not trivial. 
By that I mean proper implementation, starting from new and delete overloads 
implemeted on the class using it. `new []` and `delete []` are especially tricky. 

Also. System memory allocators are the best for combating the fragementation issue. A must for a long running processes.

Same stands for memory allocation in presence of multiple threads. It is just not feasible to compete with the system memory handling.

### Handles instead of pointers

Yes that is superior concept. But that is a paradigm shift for most of the users. And standard libraries have no such concept in use.
Thus using handles with standard API's requires a lot of to/from transformations to/from native pointers.

### We need to talk about Windows

In my nanolib I have the following

```cpp
#define DBJ_NANO_CALLOC(T_,S_) \
(T_*)::HeapAlloc(::GetProcessHeap(), 0, S_ * sizeof(T_))

#define DBJ_NANO_MALLOC(T_,S_) \
(T_*)::HeapAlloc(::GetProcessHeap(), 0, S_)

#define DBJ_NANO_FREE(P_) \
::HeapFree(::GetProcessHeap(), 0, (void*)P_)
```
It is measured many times over. Tried and tested.  On Windows there is no faster system allocation call but `HeapAlloc`. 
