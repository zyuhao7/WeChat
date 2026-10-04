#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELASINGLETON_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELASINGLETON_H_

#define Q_SINGLETON_CREATE(Class)             \
public:                                       \
    static Class* getInstance()               \
    {                                         \
        static auto instance = new Class(); \
        return instance;                      \
    }

#define Q_SINGLETON_CREATE_H(Class) \
public:                             \
    static Class* getInstance();

#define Q_SINGLETON_CREATE_CPP(Class)         \
    Class* Class::getInstance()               \
    {                                         \
        static auto instance = new Class(); \
        return instance;                      \
    }

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELASINGLETON_H_
