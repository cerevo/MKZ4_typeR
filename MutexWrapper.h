#ifndef MUTEXWRAPPER_H
#define MUTEXWRAPPER_H

//stlのmutexを使って排他処理をするだけのクラス

#include <mutex>

template <typename T>
class MutexWrapper {
public:
    MutexWrapper(const T &t):
        t(t)
    {
    }
    MutexWrapper(const MutexWrapper &r):
        t(r.get())
    {
    }
    MutexWrapper() = default;

    T get() const {
        std::lock_guard<std::mutex> lock(m);
        return t;
    }
    void set(const T &t) {
        std::lock_guard<std::mutex> lock(m);
        this->t = t;
    }

private:
    T t;
    mutable std::mutex m;
};
#endif //MUTEXWRAPPER_H
