#include <snower/actor/actor_system.h>

namespace snower
{
namespace actor
{

actor_system::actor_ref actor_system::get_actor(const actor_local_id& addr)
{
    using namespace std;
    uint64_t id = addr.get_id();
    id_actor_map_type::iterator iter;
    {
        lock_guard<mutex> locker(m_lock_maps);
        iter = m_actors.find(id);
    }
    actor_ref ret;
    if(iter != m_actors.end())
    {
        ret = get<2>(iter->second);
    }
    return move(ret);

}

void actor_system::stop(const actor_address& addr)
{
    if(addr.is_local())
    {
        erase_actor((const actor_local_id&)addr);
    }
}

bool actor_system::valid_name(const std::string& name) const
{
    return name.find_first_of("/#.@") != name.npos;
}

void actor_system::pool_mailbox(mailbox_ref mb)
{
    m_thread_pool.add_job([mb, this](){
            logger& l = singletons<logger>::get_instance("actor_system");
            mb->thread_pool_enter();
            std::function<void()> func = mb->pop();
            while(func) 
            {
                l.INFO("取到了有效的函数");
                func();
                func = mb->pop();
            };
            l.WARN("没有有效的函数了，准备结束");
            mb->thread_pool_leave();
            if(mb->add_to_pool())
            {
                pool_mailbox(mb);
            }
        });
}

uint64_t actor_system::gen_id(void)
{
    static std::atomic<uint64_t> ids(0);
    return ++ids;
}

std::string actor_system::rand_name(void)
{
    return "";
}

std::string actor_system::rand_name(uint64_t id)
{
    std::stringstream ss;
    ss << "actor#" << id;
    return std::move(ss.str());
}

actor_address actor_system::add_actor(actor_local_id* addr, class actor* a)
{
    using namespace std;
    addr_ref lid(addr, &actor_system::deletor<actor_local_id>);
    {
        lock_guard<mutex> locker(m_lock_maps);
        m_actors.emplace(addr->get_id(), forward_as_tuple(lid, mailbox_ref(new mailbox<mailbox_item>(), &actor_system::deletor<mailbox<mailbox_item>>), actor_ref(a)));
        m_name_id_map.emplace(addr->get_full_name(), addr->get_id());
    }
    actor_address ret(lid);
    a->set_self(ret);
    return move(ret);
}

void actor_system::erase_actor(const actor_local_id& addr)
{
    using namespace std;
    uint64_t id = addr.get_id();
    string name = addr.get_full_name();
    lock_guard<mutex> locker(m_lock_maps);
    auto iter = m_actors.find(id);
    if(iter != m_actors.end())
    {
        get<1>(iter->second)->clear();
    }
    m_name_id_map.erase(name);
    m_actors.erase(id);
}

actor_system::actor_bundle actor_system::get_actor_bundle(const actor_local_id& addr)
{
    using namespace std;
    uint64_t id = addr.get_id();
    id_actor_map_type::iterator iter;
    {
        lock_guard<mutex> locker(m_lock_maps);
        iter = m_actors.find(id);
    }
    actor_system::actor_bundle ret(false, nullptr, nullptr, nullptr);
    if(iter != m_actors.end())
    {
        ret = actor_bundle(true, get<0>(iter->second), get<1>(iter->second), get<2>(iter->second));
    }
    return move(ret);
}

}
}

