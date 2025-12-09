#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>

extern struct k_spinlock _sched_spinlock;

int z_impl_k_thread_weight_set(k_tid_t tid, uint32_t weight)
{
    struct k_thread *thread = tid ? tid : _current;
    K_SPINLOCK(&_sched_spinlock) {
        thread->base.prio_weight = weight;
    }
    return 0;
}

int z_impl_k_thread_weight_get(k_tid_t tid, uint32_t *weight)
{
    struct k_thread *thread = tid ? tid : _current;
    K_SPINLOCK(&_sched_spinlock) {
        *weight = thread->base.prio_weight;
    }
    return 0;
}

int z_impl_k_thread_exec_time_set(k_tid_t tid, uint32_t exec_time)
{
    struct k_thread *thread = tid ? tid : _current;
    K_SPINLOCK(&_sched_spinlock) {
        thread->base.prio_exec_time = exec_time;
    }
    return 0;
}

int z_impl_k_thread_exec_time_get(k_tid_t tid, uint32_t *exec_time)
{
    struct k_thread *thread = tid ? tid : _current;
    K_SPINLOCK(&_sched_spinlock) {
        *exec_time = thread->base.prio_exec_time;
    }
    return 0;
}

int z_impl_k_thread_time_left_set(k_tid_t tid, uint32_t time_left)
{
    struct k_thread *thread = tid ? tid : _current;
    K_SPINLOCK(&_sched_spinlock) {
        thread->base.prio_time_left = time_left;
    }
    return 0;
}

int z_impl_k_thread_time_left_get(k_tid_t tid, uint32_t *time_left)
{
    struct k_thread *thread = tid ? tid : _current;
    K_SPINLOCK(&_sched_spinlock) {
        *time_left = thread->base.prio_time_left;
    }
    return 0;
}

#ifdef CONFIG_USERSPACE

Z_SYSCALL_HANDLER(k_thread_weight_set, tid, weight)
{
    K_OOPS(K_SYSCALL_OBJ(tid, K_OBJ_THREAD));
    return z_impl_k_thread_weight_set((k_tid_t)tid, weight);
}

Z_SYSCALL_HANDLER(k_thread_weight_get, tid, weight_p)
{
    K_OOPS(K_SYSCALL_OBJ(tid, K_OBJ_THREAD));
    K_OOPS(K_SYSCALL_MEMORY_WRITE(weight_p, sizeof(uint32_t)));
    return z_impl_k_thread_weight_get((k_tid_t)tid, (uint32_t *)weight_p);
}

Z_SYSCALL_HANDLER(k_thread_exec_time_set, tid, exec_time)
{
    K_OOPS(K_SYSCALL_OBJ(tid, K_OBJ_THREAD));
    return z_impl_k_thread_exec_time_set((k_tid_t)tid, exec_time);
}

Z_SYSCALL_HANDLER(k_thread_exec_time_get, tid, exec_time_p)
{
    K_OOPS(K_SYSCALL_OBJ(tid, K_OBJ_THREAD));
    K_OOPS(K_SYSCALL_MEMORY_WRITE(exec_time_p, sizeof(uint32_t)));
    return z_impl_k_thread_exec_time_get((k_tid_t)tid, (uint32_t *)exec_time_p);
}

Z_SYSCALL_HANDLER(k_thread_time_left_set, tid, time_left)
{
    K_OOPS(K_SYSCALL_OBJ(tid, K_OBJ_THREAD));
    return z_impl_k_thread_time_left_set((k_tid_t)tid, time_left);
}

Z_SYSCALL_HANDLER(k_thread_time_left_get, tid, time_left_p)
{
    K_OOPS(K_SYSCALL_OBJ(tid, K_OBJ_THREAD));
    K_OOPS(K_SYSCALL_MEMORY_WRITE(time_left_p, sizeof(uint32_t)));
    return z_impl_k_thread_time_left_get((k_tid_t)tid, (uint32_t *)time_left_p);
}

#endif /* CONFIG_USERSPACE */
