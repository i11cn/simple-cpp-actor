#ifndef __SNOWER_ACTOR_ACTOR_ADDRESS_H__
#define __SNOWER_ACTOR_ACTOR_ADDRESS_H__

#include <memory>
#include <string>

namespace snower
{
namespace actor
{

class actor_local_id final
{
public:
    actor_local_id(const actor_local_id& addr);
    actor_local_id(actor_local_id&& addr);
    ~actor_local_id(void) = default;

    actor_local_id& operator = (const actor_local_id& addr);
    actor_local_id& operator = (actor_local_id&& addr);

private:
    actor_local_id(void);
    actor_local_id(uint64_t id, const std::string& name);
 
public:
    bool operator == (const actor_local_id& addr) const;
    bool operator != (const actor_local_id& addr) const;
    explicit operator bool (void) const;
    std::string get_name(void) const;
    std::string get_full_name(void) const;
    std::string get_parent_name(void) const;
    size_t get_id(void) const;

    friend class actor_address;
    friend class actor_system;
    friend std::ostream& operator << (std::ostream&, const actor_local_id&);

private:
    uint64_t m_id;
    std::string m_name;
};

class actor_remote_id final
{
public:
    actor_remote_id(const actor_remote_id& addr);
    actor_remote_id(actor_remote_id&& addr);
    ~actor_remote_id(void) = default;

    actor_remote_id& operator = (const actor_remote_id& addr);
    actor_remote_id& operator = (actor_remote_id&& addr);

private:
    actor_remote_id(void);

public:
    bool operator == (const actor_remote_id& addr) const;
    bool operator != (const actor_remote_id& addr) const;
    explicit operator bool (void) const;

    friend class actor_address;
    friend class actor_system;
    friend std::ostream& operator << (std::ostream&, const actor_remote_id&);
};

class actor_address final
{
public:
    static const actor_local_id INVALID_LOCAL_ID;
    static const actor_remote_id INVALID_REMOTE_ID;

public:
    actor_address(void);
    actor_address(const actor_address& addr);
    actor_address(actor_address&& addr);
    ~actor_address(void);

    actor_address& operator = (const actor_address& addr);
    actor_address& operator = (actor_address&& addr);
    bool operator == (const actor_address& addr) const;
    bool operator != (const actor_address& addr) const;
    explicit operator bool (void) const;
    bool is_remote(void) const;
    bool is_local(void) const;
    operator actor_local_id(void) const;
    operator actor_remote_id(void) const;

private:
    actor_address(const std::shared_ptr<actor_local_id>& lid);
    actor_address(const std::shared_ptr<actor_local_id>& lid, const std::shared_ptr<actor_remote_id>& rid);

private:
    std::weak_ptr<actor_local_id> m_local_id;
    std::weak_ptr<actor_remote_id> m_remote_id;

    friend class actor_system;
    friend std::ostream& operator << (std::ostream&, const actor_address&);
};

} // namespace actor
} // namespace snower

#endif // __SNOWER_ACTOR_ACTOR_ADDRESS_H__

