#include "RemoveStateFromEvent.hpp"

#include "Document/Event/EventModel.hpp"

using namespace iscore;
using namespace Scenario::Command;

RemoveStateFromEvent::RemoveStateFromEvent(ObjectPath &&eventPath, const State& state):
    SerializableCommand {"ScenarioControl",
                         className(),
                         description()},
    m_path {std::move(eventPath) },
    m_state{state}
{

}

void RemoveStateFromEvent::undo()
{
    auto event = m_path.find<EventModel>();
    event->addState(m_state);
}

void RemoveStateFromEvent::redo()
{
    auto event = m_path.find<EventModel>();
    event->removeState(m_state);
}

bool RemoveStateFromEvent::mergeWith(const Command* other)
{
    return false;
}

void RemoveStateFromEvent::serializeImpl(QDataStream& s) const
{
    s << m_path << m_state;
}

void RemoveStateFromEvent::deserializeImpl(QDataStream& s)
{
    s >> m_path >> m_state;
}
