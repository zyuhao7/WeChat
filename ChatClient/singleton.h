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
            //单例通常用来管理全局资源，而使用 std::make_shared 时，管理对象和实例绑定在一起，可能会导致不易控制的析构顺序问题。
            // make_shared<>() 会 申请一块连续内存，同时存储 T 对象和控制块（引用计数等）导致 T 和控制块（引用计数）绑死了
            // 析构时，T 必须先销毁，控制块再销毁（顺序不可控)

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
