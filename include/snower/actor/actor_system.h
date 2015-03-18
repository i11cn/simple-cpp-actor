#ifndef __SNOWER_ACTOR_ACTOR_SYSTEM_H__
#define __SNOWER_ACTOR_ACTOR_SYSTEM_H__

#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>
#include <snower/singleton.h>
#include <snower/thread_pool.h>
#include <snower/actor/actor_address.h>
#include <snower/actor/mailbox.h>

namespace snower
{
namespace actor
{

class actor_system
{
private:
    using addr_ref = std::shared_ptr<class actor_local_id>;
    using mailbox_item = std::function<void()>;
    using mailbox_ref = std::shared_ptr<mailbox<mailbox_item>>;
    using actor_ref = std::shared_ptr<class actor>;
    using actor_map_item = std::tuple<addr_ref, mailbox_ref, actor_ref>;
    using id_actor_map_type = std::unordered_map<uint64_t, actor_map_item>;
    using actor_bundle = std::tuple<bool, addr_ref, mailbox_ref, actor_ref>;

private:
    actor_system(void) {}
    ~actor_system(void) {}

public:
    actor_ref get_actor(const actor_local_id& addr);
    void stop(const class actor_address& addr);
    bool valid_name(const std::string& name) const;

    template<typename Actor, typename... Types>
    actor_address spawn(Types&&... args);
    template<typename Actor, typename... Types>
    actor_address spawned_by(const class actor_address& parent, Types&&... args);
    template<typename Actor, typename... Types>
    actor_address spawn_and_named(const std::string& name, Types&&... args);
    template<typename Actor, typename... Types>
    actor_address spawn_and_named_by(const actor_address& parent, const std::string& name, Types&&... args);
    template<typename... Types>
    void send(const actor_address& addr, Types&&... args);
    template<typename... Types>
    void send_as(const actor_address& sender, const actor_address& receiver, Types&&... args);

private:
    void pool_mailbox(mailbox_ref mb);
    uint64_t gen_id(void);
    std::string rand_name(void);
    std::string rand_name(uint64_t id);
    actor_address add_actor(actor_local_id* addr, class actor* a);
    void erase_actor(const actor_local_id& addr);
    actor_bundle get_actor_bundle(const actor_local_id& addr);

    template<typename T>
    static void deletor(T* p)
    {
        delete p;
    }

private:
    id_actor_map_type m_actors;
    std::unordered_map<std::string, uint64_t> m_name_id_map;
    std::mutex m_lock_maps;
    thread_pool<Sequence> m_thread_pool;
    template<typename Actor>
    friend actor_address spawn(void);
    friend class singleton<actor_system>;
};

template<typename Actor, typename... Types>
inline actor_address spawn(Types&&... args)
{
    actor_system& as = singleton<actor_system>::get_instance();
    return as.spawn<Actor, Types...>(std::forward<Types>(args)...);
}

template<typename Actor, typename... Types>
inline actor_address spawned_by(const actor_address& addr, Types&&... args)
{
    actor_system& as = singleton<actor_system>::get_instance();
    return as.spawn<Actor, Types...>(std::forward<Types>(args)...);
}

template<typename Actor, typename... Types>
inline actor_address spawn_and_named(const std::string& name, Types&&... args)
{
    actor_system& as = singleton<actor_system>::get_instance();
    return as.spawn_and_named<Actor, Types...>(name, std::forward<Types>(args)...);
}

template<typename Actor, typename... Types>
inline actor_address spawn_and_named_by(const actor_address& addr, const std::string& name, Types&&... args)
{
    actor_system& as = singleton<actor_system>::get_instance();
    return as.spawn_and_named<Actor, Types...>(name, std::forward<Types>(args)...);
}

inline void stop(const actor_address& addr)
{
    actor_system& as = singleton<actor_system>::get_instance();
    as.stop(addr);
}

template<typename... Types>
void send(const actor_address& receiver, Types... args)
{
    actor_system& as = singleton<actor_system>::get_instance();
    as.send(receiver, std::forward<Types>(args)...);
}
template<typename... Types>
void send_as(const actor_address& sender, const actor_address& receiver, Types... args)
{
    actor_system& as = singleton<actor_system>::get_instance();
    as.send_as(sender, receiver, std::forward<Types>(args)...);
}

void shutdown(void);
void wait_for_all_actor_done(void);

} // namespace actor
} // namespace snower 

#include <snower/actor/actor_system.inl>

#endif // __SNOWER_ACTOR_ACTOR_SYSTEM_H__

