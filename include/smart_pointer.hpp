#ifndef __EASYWORK__SMART_POINTER_HPP__
#define __EASYWORK__SMART_POINTER_HPP__



#include <pthread.h>

#include "common.hpp"

namespace easywork {

namespace RefCounter_ {
class RefCounter;
}
typedef RefCounter_::RefCounter RefCounter;

namespace SmartPointer_ {
template <typename T> class SmartPointer;
}

}


namespace easywork {

namespace RefCounter_ {
class RefCounter
{
public:
    pthread_spinlock_t count_locker;
    int count;

    explicit RefCounter(void)
        : count(0)
    {
        if (::pthread_spin_init(&this->count_locker,
                                PTHREAD_PROCESS_PRIVATE)) {
            ::abort(); // out of resource
        }

        return;
    }

    ~RefCounter(void)
    {
        static_cast<void>(::pthread_spin_destroy(&this->count_locker));

        return;
    }
};
}

namespace SmartPointer_ {
template <typename T> class SmartPointer
{
public:
    SmartPointer(void)
        : obj(NULL)
    {}

    explicit SmartPointer(T *holder)
        : obj(holder)
    {
        if (NULL == this->obj) {
            return;
        }

        ++(this->obj->ref_counter.count);
    }

    ~SmartPointer(void)
    {
        if (this->obj) {
            Release_();
        }
    }

    SmartPointer(SmartPointer const &other)
        : obj(NULL)
    {
        if (! const_cast<SmartPointer &>(other).IsNull()) {
            Deliver_(other);
        }
    }

    SmartPointer const &operator =(SmartPointer const &other)
    {
        if (this == &other) {
            return other;
        }

        // 释放现有obj
        if (this->obj) {
            Release_();
        }

        // 执行传递
        if (! const_cast<SmartPointer &>(other).IsNull()) {
            Deliver_(other);
        }

        return *this;
    }

    bool IsNull(void)
    {
        return NULL == this->obj;
    }

    operator T *()
    {
        return this->obj;
    }

    T *operator ->()
    {
        return this->obj;
    }

private:
    void Deliver_(SmartPointer const &other)
    {
        // 接受新obj
        static_cast<void>(::pthread_spin_lock(
            &other.obj->ref_counter.count_locker
        ));
        ++other.obj->ref_counter.count;
        this->obj = other.obj;
        static_cast<void>(::pthread_spin_unlock(
            &other.obj->ref_counter.count_locker
        ));

        return;
    }

    void Release_(void)
    {
        int need_delete;

        // 临界区 {{
        static_cast<void>(::pthread_spin_lock(
            &this->obj->ref_counter.count_locker
        ));
        --this->obj->ref_counter.count;
        need_delete = (0 == this->obj->ref_counter.count);
        static_cast<void>(::pthread_spin_unlock(
            &this->obj->ref_counter.count_locker
        ));
        // }} 临界区

        if (need_delete) {
            delete this->obj;
        }
        this->obj = NULL;

        return;
    }

private:
    T *obj;
};
}

}


// SMART_POINTABLE不可用于local class
#define SMART_POINTABLE       \
        template <typename T> \
        friend class ::easywork::SmartPointer_::SmartPointer;\
        private:\
            /* you should use std::vector as a single object */\
            void *operator new[](size_t size);\
            void operator delete[](void *p);\
        private:\
            ::easywork::RefCounter ref_counter;
#endif // __EASYWORK__SMART_POINTER_HPP__
