#include <iostream>
#include <snower/actor/actor_address.h>

using namespace std;

namespace snower
{
namespace actor
{

actor_local_id::actor_local_id(void)
: m_id(0)
, m_name("")
{
}

actor_local_id::actor_local_id(uint64_t id, const std::string& name)
: m_id(id)
, m_name(name)
{
}

actor_local_id::actor_local_id(const actor_local_id& addr)
: m_id(addr.m_id)
, m_name(addr.m_name)
{
}

actor_local_id::actor_local_id(actor_local_id&& addr)
: m_id(addr.m_id)
, m_name(std::move(addr.m_name))
{
}

actor_local_id& actor_local_id::operator = (const actor_local_id& addr)
{
    m_id = addr.m_id;
    m_name = addr.m_name;
    return *this;
}

actor_local_id& actor_local_id::operator = (actor_local_id&& addr)
{
    m_id = addr.m_id;
    m_name = std::move(addr.m_name);
    return *this;
}

bool actor_local_id::operator == (const actor_local_id& addr) const
{
    return (m_id == addr.m_id) && (m_name == addr.m_name);
}

bool actor_local_id::operator != (const actor_local_id& addr) const
{
    return !(operator == (addr));
}

actor_local_id::operator bool (void) const
{
    return (m_id != 0) && !m_name.empty();
}

std::string actor_local_id::get_name(void) const
{
    auto pos = m_name.find_last_of("/");
    std::string ret = (pos != m_name.npos) ? m_name.substr(pos + 1) : m_name;
    return std::move(ret);
}

std::string actor_local_id::get_full_name(void) const
{
    return m_name;
}

std::string actor_local_id::get_parent_name(void) const
{
    std::string ret = "/";
    auto pos = m_name.find_last_of("/");
    if(pos != m_name.npos && pos > 0)
    {
        ret = m_name.substr(0, pos);
    }
    return std::move(ret);
}

size_t actor_local_id::get_id(void) const
{
    return m_id;
}

actor_remote_id::actor_remote_id(void)
{
}

actor_remote_id::actor_remote_id(const actor_remote_id& addr)
{
}

actor_remote_id::actor_remote_id(actor_remote_id&& addr)
{
}

actor_remote_id& actor_remote_id::operator = (const actor_remote_id& addr)
{
    return *this;
}

actor_remote_id& actor_remote_id::operator = (actor_remote_id&& addr)
{
    return *this;
}

bool actor_remote_id::operator == (const actor_remote_id& addr) const
{
    return false;
}

bool actor_remote_id::operator != (const actor_remote_id& addr) const
{
    return !(operator == (addr));
}

actor_remote_id::operator bool (void) const
{
    return false;
}

actor_address::actor_address(void)
{
}

actor_address::actor_address(const actor_address& addr)
: m_local_id(addr.m_local_id)
, m_remote_id(addr.m_remote_id)
{
}

actor_address::actor_address(actor_address&& addr)
: m_local_id(std::move(addr.m_local_id))
, m_remote_id(std::move(addr.m_remote_id))
{
}

actor_address::actor_address(const std::shared_ptr<actor_local_id>& lid)
: m_local_id(lid)
{
}

actor_address::actor_address(const std::shared_ptr<actor_local_id>& lid, const std::shared_ptr<actor_remote_id>& rid)
: m_local_id(lid)
, m_remote_id(rid)
{
}

actor_address::~actor_address(void)
{
}

actor_address& actor_address::operator = (const actor_address& addr)
{
    m_local_id = addr.m_local_id;
    m_remote_id = addr.m_remote_id;
    return *this;
}

actor_address& actor_address::operator = (actor_address&& addr)
{
    m_local_id = std::move(addr.m_local_id);
    m_remote_id = std::move(addr.m_remote_id);
    return *this;
}

bool actor_address::operator == (const actor_address& addr) const
{
    std::shared_ptr<actor_local_id> lid = m_local_id.lock();
    actor_local_id& olid = (actor_local_id&)addr;
    if(lid)
    {
        if(!olid || olid != *lid) return false;
    }
    else
    {
        if(olid) return false;
    }
    std::shared_ptr<actor_remote_id> rid = m_remote_id.lock();
    actor_remote_id& orid = (actor_remote_id&)addr;
    if(rid)
    {
        if(!orid || orid != *rid) return false;
    }
    else
    {
        if(orid) return false;
    }
    return true;
}

bool actor_address::operator != (const actor_address& addr) const
{
    return !(operator == (addr));
}

actor_address::operator bool (void) const
{
    std::shared_ptr<actor_local_id> id = m_local_id.lock();
    return id && ((bool)(*id));
}

bool actor_address::is_remote(void) const
{
    std::shared_ptr<actor_remote_id> id = m_remote_id.lock();
    return (bool)id && (bool)(*id);
}

bool actor_address::is_local(void) const
{
    return operator bool() && !is_remote();
}

actor_address::operator actor_local_id(void) const
{
    std::shared_ptr<actor_local_id> ret = m_local_id.lock();
    if(ret)
    {
        return std::move(actor_local_id(*ret));
    }
    else
    {
        return INVALID_LOCAL_ID;
    }
}

actor_address::operator actor_remote_id(void) const
{
    std::shared_ptr<actor_remote_id> ret = m_remote_id.lock();
    if(ret)
    {
        return std::move(actor_remote_id(*ret));
    }
    else
    {
        return INVALID_REMOTE_ID;
    }
}

std::ostream& operator << (std::ostream& os, const actor_remote_id& rid)
{
    if(rid)
    {
        cout << "NOT IMPLEMENT";
    }
    else
    {
        os << "INVALID REMOTE ID";
    }
    return os;
}

std::ostream& operator << (std::ostream& os, const actor_local_id& lid)
{
    if(lid)
    {
        cout << lid.m_name << "#" << lid.m_id;
    }
    else
    {
        os << "INVALID LOCAL ID";
    }
    return os;
}

std::ostream& operator << (std::ostream& os, const actor_address& addr)
{
    actor_local_id lid = addr;
    if(lid)
    {
        os << lid;
    }
    else
    {
        os << "-";
    }
    os << "@";
    actor_remote_id rid = addr;
    if(rid)
    {
        os << rid;
    }
    else
    {
        os << "-";
    }
    return os;
}

const actor_local_id actor_address::INVALID_LOCAL_ID;
const actor_remote_id actor_address::INVALID_REMOTE_ID;

} // namespace actor
} // namespace snower

