#ifndef SINGLETON_H
#define SINGLETON_H
#include <global.h>

template<typename T>
class Singleton{
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator= (const Singleton<T>& st) = delete;

    static std::shared_ptr<T> _instance;

public:
    static std::shared_ptr<T> GetInstance(){
        static std::once_flag s_flag;
        std::call_once(s_flag,[&](){
            // make_shared() ???
            //A singleton usually manages global resources; with std::make_shared the manager and instance are bound together, which can cause hard-to-control destruction-order issues.
            // make_shared<>() allocates one contiguous block storing both the T object and the control block (refcount), tying T and the control block together
            // on destruction T must be destroyed before the control block (order not controllable)

            _instance = std::shared_ptr<T>(new T);
        });
       return _instance;
    }

    void PrintAddress()
    {
        std::cout<<_instance.get()<<std::endl;
    }

    ~Singleton()
    {
        std::cout<<"this is singleton destruct"<<std::endl;
    }
};

template<typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;


#endif // SINGLETON_H
