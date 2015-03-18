#include <snower/actor/actor.h>

namespace snower
{
namespace actor
{

actor::actor(void) {}

actor::~actor(void) {}

void actor::spawn(void)
{
}

const actor_address& actor::get_sender(void) const
{
    return m_sender;
}

const actor_address& actor::get_self(void) const
{
    return m_self;
}

const void actor::enroll_creator(void) const
{
}

void actor::quit(void)
{
}

void actor::set_self(const actor_address& addr)
{
    m_self = addr;
}

void actor::set_sender(const actor_address& addr)
{
    m_sender = addr;
}

void actor::reset_sender(void)
{
    m_sender = actor_address();
}

}
}

