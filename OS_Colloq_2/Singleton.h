#pragma once
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <cstdlib>
#include <cassert>

template <typename T>
struct CreateUsingNew 
{
    static T* Create() { return new T(); }
    static void Destroy(T* obj) { delete obj; }
};

template <typename T>
struct CreateStatic 
{
    static T* Create() 
    {
        static T instance;
        return &instance;
    }
    static void Destroy(T* obj) { obj->~T; }
};

template <typename T>
class DefaultLifetime 
{
public:
    static void ScheduleDestruction(void (*func)()) 
    {
        std::atexit(func);
    }
    static void OnDeadReference()
    {
        throw std::logic_error("Dead Reference Detected");
    }
};

template <typename T>
class PhoenixSingleton 
{
public:
    static void ScheduleDestruction(void (*func)()) 
    {
        if (!destroyedOnce_)
            std::atexit(func);
    }
    static void OnDeadReference()
    {
        destroyedOnce_ = true;
    }
private:
    static bool destroyedOnce_;
};

template <class T> bool PhoenixSingleton<T>::destroyedOnce_ = false;

template <typename T>
class NoDestroy 
{
public:
    static void ScheduleDestruction(void (*func)()) {}
    static void OnDeadReference() {}
};


template <typename T>
class SingleThreaded 
{
public:
    struct LockGuard { LockGuard() {} };
    using MutexType = void;
};

template <typename T>
class ClassLevelLockable 
{
public:
    struct LockGuard 
    {
        LockGuard() { GetMutex().lock(); }
        ~LockGuard() { GetMutex().unlock(); }
    };

    using MutexType = std::mutex;

    static std::mutex& GetMutex() 
    {
        static std::mutex mtx;
        return mtx;
    }
};

template <
    typename T,
    template <typename> class CreationPolicy = CreateUsingNew,
    template <typename> class LifetimePolicy = DefaultLifetime,
    template <typename> class ThreadingModel = ClassLevelLockable
>
class Singleton 
{
public:
    static T& Instance() {
        T* instance = pInstance_.load(std::memory_order_acquire);

        if (!instance) 
        {
            typename ThreadingModel<T>::LockGuard lock;
            instance = pInstance_.load(std::memory_order_relaxed);

            if (!instance) {
                if (destroyed_)
                {
                    LifetimePolicy<T>::OnDeadReference();
                    destroyed_ = false;
                }
                instance = CreationPolicy<T>::Create();
                pInstance_.store(instance, std::memory_order_release);
                LifetimePolicy<T>::ScheduleDestruction(&DestroySingleton);
            }
        }
        return *instance;
    }

private:
    Singleton() = delete;
    ~Singleton() = delete;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    static void DestroySingleton() 
    {
        assert(!destroyed_);
        CreationPolicy<T>::Destroy(pInstance_.load());
        pInstance_.store(nullptr);
        destroyed_ = true;
    }

    static std::atomic<T*> pInstance_;
    static bool destroyed_;
};

template <typename T, template <typename> class CP,
    template <typename> class LP, template <typename> class TM>
std::atomic<T*> Singleton<T, CP, LP, TM>::pInstance_{ nullptr };

template <typename T, template <typename> class CP,
    template <typename> class LP, template <typename> class TM>
bool Singleton<T, CP, LP, TM>::destroyed_ = false;