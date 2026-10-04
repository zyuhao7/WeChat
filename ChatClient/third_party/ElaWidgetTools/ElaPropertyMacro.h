#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELAPROPERTY_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELAPROPERTY_H_

#include <QSharedPointer>
#include <QtCore/qglobal.h>

#define Q_PROPERTY_CREATE(TYPE, M)                          \
    Q_PROPERTY(TYPE p##M MEMBER _p##M NOTIFY p##M##Changed) \
public:                                                     \
    Q_SIGNAL void p##M##Changed();                          \
    void set##M(TYPE M)                                     \
    {                                                       \
        _p##M = M;                                          \
        Q_EMIT p##M##Changed();                             \
    }                                                       \
    Q_REQUIRED_RESULT TYPE get##M() const                   \
    {                                                       \
        return _p##M;                                       \
    }                                                       \
                                                            \
private:                                                    \
    TYPE _p##M;

#define Q_PROPERTY_REF_CREATE(TYPE, M)                      \
    Q_PROPERTY(TYPE p##M MEMBER _p##M NOTIFY p##M##Changed) \
public:                                                     \
    Q_SIGNAL void p##M##Changed();                          \
    void set##M(const TYPE& M)                              \
    {                                                       \
        _p##M = M;                                          \
        Q_EMIT p##M##Changed();                             \
    }                                                       \
    Q_REQUIRED_RESULT const TYPE& get##M() const            \
    {                                                       \
        return _p##M;                                       \
    }                                                       \
                                                            \
private:                                                    \
    TYPE _p##M;

#define Q_PROPERTY_CREATE_Q_H(TYPE, M)                                  \
    Q_PROPERTY(TYPE p##M READ get##M WRITE set##M NOTIFY p##M##Changed) \
public:                                                                 \
    Q_SIGNAL void p##M##Changed();                                      \
    void set##M(TYPE M);                                                \
    Q_REQUIRED_RESULT TYPE get##M() const;

#define Q_PROPERTY_REF_CREATE_Q_H(TYPE, M)                              \
    Q_PROPERTY(TYPE p##M READ get##M WRITE set##M NOTIFY p##M##Changed) \
public:                                                                 \
    Q_SIGNAL void p##M##Changed();                                      \
    void set##M(const TYPE& M);                                         \
    Q_REQUIRED_RESULT const TYPE& get##M() const;

#define Q_PRIVATE_CREATE_Q_H(TYPE, M) \
public:                               \
    void set##M(TYPE M);              \
    Q_REQUIRED_RESULT TYPE get##M() const;

#define Q_PRIVATE_REF_CREATE_Q_H(TYPE, M) \
public:                                   \
    void set##M(const TYPE& M);           \
    Q_REQUIRED_RESULT const TYPE& get##M() const;

#define Q_PROPERTY_CREATE_Q_CPP(CLASS, TYPE, M) \
    void CLASS::set##M(TYPE M)                  \
    {                                           \
        Q_D(CLASS);                             \
        d->_p##M = M;                           \
        Q_EMIT p##M##Changed();                 \
    }                                           \
    TYPE CLASS::get##M() const                  \
    {                                           \
        return d_ptr->_p##M;                    \
    }

#define Q_PROPERTY_REF_CREATE_Q_CPP(CLASS, TYPE, M) \
    void CLASS::set##M(const TYPE& M)               \
    {                                               \
        Q_D(CLASS);                                 \
        d->_p##M = M;                               \
        Q_EMIT p##M##Changed();                     \
    }                                               \
    const TYPE& CLASS::get##M() const               \
    {                                               \
        return d_ptr->_p##M;                        \
    }

#define Q_PRIVATE_CREATE_Q_CPP(CLASS, TYPE, M) \
    void CLASS::set##M(TYPE M)                 \
    {                                          \
        Q_D(CLASS);                            \
        d->_p##M = M;                          \
    }                                          \
    TYPE CLASS::get##M() const                 \
    {                                          \
        return d_ptr->_p##M;                   \
    }

#define Q_PRIVATE_REF_CREATE_Q_CPP(CLASS, TYPE, M) \
    void CLASS::set##M(const TYPE& M)              \
    {                                              \
        Q_D(CLASS);                                \
        d->_p##M = M;                              \
    }                                              \
    const TYPE& CLASS::get##M() const              \
    {                                              \
        return d_ptr->_p##M;                       \
    }

#define Q_PROPERTY_CREATE_D(TYPE, M) \
private:                             \
    TYPE _p##M;

#define Q_PRIVATE_CREATE_D(TYPE, M) \
private:                            \
    TYPE _p##M;

#define Q_PRIVATE_CREATE(TYPE, M)         \
public:                                   \
    void set##M(TYPE M)                   \
    {                                     \
        _p##M = M;                        \
    }                                     \
    Q_REQUIRED_RESULT TYPE get##M() const \
    {                                     \
        return _p##M;                     \
    }                                     \
                                          \
private:                                  \
    TYPE _p##M;

#define Q_PRIVATE_REF_CREATE(TYPE, M)            \
public:                                          \
    void set##M(const TYPE& M)                   \
    {                                            \
        _p##M = M;                               \
    }                                            \
    Q_REQUIRED_RESULT const TYPE& get##M() const \
    {                                            \
        return _p##M;                            \
    }                                            \
                                                 \
private:                                         \
    TYPE _p##M;

#define Q_Q_CREATE(CLASS)                                          \
protected:                                                         \
    QSharedPointer<CLASS##Private> d_ptr;                          \
                                                                   \
private:                                                           \
    Q_DISABLE_COPY(CLASS)                                          \
    Q_DECLARE_PRIVATE(CLASS);                                      \
    void _set##CLASS##Private(QSharedPointer<CLASS##Private> ptr); \
    Q_REQUIRED_RESULT QSharedPointer<CLASS##Private> _get##CLASS##Private() const;

#define Q_Q_CREATE_CPP(CLASS)                                            \
    void CLASS::_set##CLASS##Private(QSharedPointer<CLASS##Private> ptr) \
    {                                                                    \
        d_ptr = std::move(ptr);                                          \
        d_ptr->q_ptr = this;                                             \
    }                                                                    \
    QSharedPointer<CLASS##Private> CLASS::_get##CLASS##Private() const   \
    {                                                                    \
        return d_ptr;                                                    \
    }

#define Q_D_CREATE(CLASS) \
protected:                \
    CLASS* q_ptr;         \
                          \
private:                  \
    Q_DECLARE_PUBLIC(CLASS);

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELAPROPERTY_H_
