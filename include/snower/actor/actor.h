#ifndef __SNOWER_ACTOR_ACTOR_H__
#define __SNOWER_ACTOR_ACTOR_H__

#include <functional>
#include <map>
#include <snower/actor/actor_address.h>

namespace snower
{
namespace actor
{

class actor
{
public:
    actor(void);
    virtual ~actor(void);

protected:
    void spawn(void);
    const actor_address& get_sender(void) const;
    const actor_address& get_self(void) const;
    const void enroll_creator(void) const;
    void quit(void);

    template<typename... Types>
    void handle(void(*func)(Types...));
    template<typename Class, typename... Types>
    void handle(void(Class::* func)(Types...), Class* c);
    template<typename... Types>
    void unhandle(void);
    template<typename... Types>
    void reply(Types... args);
    template<typename... Types>
    void send(const actor_address& receiver, Types... args);
    template<typename... Types>
    void forward(const actor_address& next, Types... args);

private:
    void set_self(const actor_address& addr);
    void set_sender(const actor_address& addr);
    void reset_sender(void);

    template<typename... Types>
    bool call(Types&&... args);
    template<typename... Types>
    bool call(std::tuple<Types...>* t);
    template<typename... Types>
    std::function<void()> bind(Types... args);

private:
    actor_address m_sender;
    actor_address m_self;
    std::map<size_t, class caller> m_handlers;

    template<typename Actor>
    friend void set_sender(Actor&, const actor_address&);
    template<typename Actor>
    friend void set_self(Actor&, const actor_address&);
    friend class actor_system;
};

} // namespace actor
} // namespace snower

#include <snower/actor/actor.inl>

#endif // __SNOWER_ACTOR_ACTOR_H__

