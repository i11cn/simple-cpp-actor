#ifndef __SNOWER_SINGLETON_H__
#define __SNOWER_SINGLETON_H__

#include <map>
#include <mutex>
#include <string>

namespace snower
{

namespace instantiate_policy
{

class Lazy {};
class Normal {};

} // namespace instantiate_policy

template<typename Impl, typename Policy = instantiate_policy::Normal>
class singleton final
{
};

template<typename Impl>
class singleton<Impl, instantiate_policy::Lazy> final
{
private:
    singleton(void) {}
    ~singleton(void) {}

public:
    static Impl& get_instance(void)
    {
        static Impl instance;
        return instance;
    }
};

template<typename Impl>
class singleton<Impl, instantiate_policy::Normal> final
{
private:
    struct object_creator
    {
        object_creator(void)
        {
            singleton<Impl, instantiate_policy::Normal>::get_instance();
        }
        inline void dummy(void) const {}
    };

    static object_creator m_oc;

private:
    singleton(void) {}
    ~singleton(void) {}

public:
    static Impl& get_instance(void)
    {
        static Impl instance;
        m_oc.dummy();
        return instance;
    }
};

template<typename Impl>
typename singleton<Impl, instantiate_policy::Normal>::object_creator singleton<Impl, instantiate_policy::Normal>::m_oc;

class PassName {};
class NotPassName {};

template<typename Type, typename Create = PassName>
class singletons final
{
};

template<typename Type>
class singletons<Type, PassName> final
{
private:
    singletons(void) {}
    ~singletons(void) {}

public:
    static Type& get_instance(const std::string& name = "")
    {
        using namespace std;
        static map<string, Type> instances;
        static mutex lock;

        auto iter = instances.find(name);
        if(iter == instances.end())
        {
            lock_guard<mutex> locker(lock);
            iter = instances.find(name);
            if(iter != instances.end())
            {
                return iter->second;
            }
            else
            {
                instances.emplace(name, Type(name));
                return instances.find(name)->second;
            }
        }
        else
        {
            return iter->second;
        }
    }
};

template<typename Type>
class singletons<Type, NotPassName> final
{
private:
    singletons(void) {}
    ~singletons(void) {}

public:
    static Type& get_instance(const std::string& name = "")
    {
        using namespace std;
        static map<string, Type> instances;
        static mutex lock;

        auto iter = instances.find(name);
        if(iter == instances.end())
        {
            lock_guard<mutex> locker(lock);
            iter = instances.find(name);
            if(iter != instances.end())
            {
                return iter->second;
            }
            else
            {
                instances.emplace(name, Type());
                return instances[name];
            }
        }
        else
        {
            return iter->second;
        }
    }
};

} // namespace snower

#endif // __SNOWER_SINGLETON_H__

