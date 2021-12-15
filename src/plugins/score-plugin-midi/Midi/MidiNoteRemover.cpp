#include "MidiNoteRemover.hpp"

#include <Midi/Commands/RemoveNotes.hpp>
#include <Midi/MidiNote.hpp>
#include <Midi/MidiProcess.hpp>

#include <score/command/Dispatchers/CommandDispatcher.hpp>
#include <score/document/DocumentContext.hpp>
namespace Midi
{
bool NoteRemover::remove(const Selection& s, const score::DocumentContext& ctx)
{
  if (!s.empty())
  {
    std::vector<Id<Note>> noteIdList;
    for (auto item : s)
    {
      if (auto model = qobject_cast<const Midi::Note*>(item.data()))
      {
        if (auto parent
            = qobject_cast<const Midi::ProcessModel*>(model->parent()))
        {
          noteIdList.push_back(model->id());
        }
      }
    }
    if (!noteIdList.empty())
    {
      auto parent = qobject_cast<const Midi::ProcessModel*>(
          s.begin()->data()->parent());
      CommandDispatcher<>{ctx.commandStack}.submit<Midi::RemoveNotes>(
          *parent, noteIdList);
      return true;
    }
  }
  return false;
}
}
