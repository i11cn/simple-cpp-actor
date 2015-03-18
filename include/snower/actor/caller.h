#ifndef __SNOWER_ACTOR_CALLER_H__
#define __SNOWER_ACTOR_CALLER_H__

#include <functional>

namespace snower
{
namespace actor
{

template<int N>
class my_placeholders
{
};

} // namespace actor
} // namespace snower

namespace std
{

template<size_t N>
struct is_placeholder<snower::actor::my_placeholders<N>> : public integral_constant<int, N> {};

}

namespace snower
{
namespace actor
{

template<size_t...>
struct seqs {};
template<size_t N, size_t... S>
struct gen_seqs : gen_seqs<N - 1, N, S...> {};
template<size_t... S>
struct gen_seqs<1, S...>
{
    typedef seqs<1, S...> seqs_type;
};

template<typename F, typename C, size_t... S>
auto bind_args(F f, C* c, seqs<S...>) -> decltype(std::bind(f, c, my_placeholders<S>()...))
{
    return std::bind(f, c, my_placeholders<S>()...);
}

class caller
{
private:
    class placeholder
    {
    public:
        virtual ~placeholder(void) {}
    };

    template<typename... Types>
    class holder : public placeholder
    {
    public:
        holder(void) = delete;
        holder(std::function<void (Types...)>&& func)
        : m_func(std::move(func))
        {
        }
        void call(Types... args)
        {
            m_func(std::forward<Types>(args)...);
        }
        void call(std::tuple<Types...>& t)
        {
            call_for_tuple(t, std::make_index_sequence<sizeof...(Types)>());
        }

    private:
        template<size_t... Seq>
        void call_for_tuple(std::tuple<Types...>& t)
        {
            m_func(std::get<Seq>(t)...);
        }

    private:
        std::function<void (Types...)> m_func;
    };

public:
    caller(void) = delete;
    template<typename... Types>
    caller(std::function<void (Types...)>&& func)
    : m_holder(new holder<Types...>(std::move(func)))
    , m_hash(caller_hash<Types...>())
    {
    }
    caller(caller&& c)
    : m_holder(c.m_holder)
    , m_hash(c.m_hash)
    {
        c.m_holder = nullptr;
    }
    ~caller(void)
    {
        if(m_holder != nullptr)
        {
            delete m_holder;
        }
        m_holder = nullptr;
    };
    caller& operator = (caller&& c)
    {
        m_holder = c.m_holder;
        m_hash = c.m_hash;
        c.m_holder = nullptr;
        return *this;
    }
    template<typename T>
    caller& operator = (T) = delete;
    size_t hash_code(void) const
    {
        return m_hash;
    }
    template<typename... Types>
    void operator () (Types... args)
    {
        if(caller_hash<Types...>() == m_hash)
        {
            holder<Types...>* h = (holder<Types...>*)m_holder;
            h->call(std::forward<Types>(args)...);
        }
    }
    template<typename... Types>
    static constexpr size_t caller_hash(void)
    {
        return typeid(holder<Types...>).hash_code();
    }

private:
    placeholder* m_holder;
    size_t m_hash;
};

} // namespace actor
} // namespace snower

#endif // __SNOWER_ACTOR_CALLER_H__

