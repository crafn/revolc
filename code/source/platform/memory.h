#ifndef REVOLC_PLATFORM_MEMORY_H
#define REVOLC_PLATFORM_MEMORY_H

/// @todo Other compilers
#define PLAT_FULL_MEMORY_BARRIER() __sync_synchronize()
#define PLAT_ACQUIRE_FENCE() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define PLAT_RELEASE_FENCE() __atomic_thread_fence(__ATOMIC_RELEASE)

#endif // REVOLC_PLATFORM_MEMORY_H
