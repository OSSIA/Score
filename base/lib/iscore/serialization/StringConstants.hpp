#pragma once
#include <QString>
#include <iscore_lib_base_export.h>
namespace iscore
{
struct StringConstants
{
  const QString k;
  const QString v;
  const QString id;
  const QString none;
  const QString Identifiers;
  const QString Type;
  const QString Value;
  const QString Address;
  const QString Message;
  const QString value;
  const QString address;
  const QString LHS;
  const QString Op;
  const QString RHS;
  const QString Previous;
  const QString Following;
  const QString User;
  const QString Priorities;
  const QString Process;
  const QString Name;
  const QString ObjectName;
  const QString ObjectId;
  const QString Children;
  const QString Min;
  const QString Max;
  const QString Values;
  const QString Device;
  const QString Path;
  const QString ioType;
  const QString ClipMode;
  const QString Unit;
  const QString unit;
  const QString RepetitionFilter;
  const QString RefreshRate;
  const QString Priority;
  const QString Tags;
  const QString Domain;
  const QString Protocol;
  const QString Duration;
  const QString Metadata;
  const QString lowercase_true;
  const QString lowercase_false;
  const QString True;
  const QString False;
  const QString lowercase_yes;
  const QString lowercase_no;
  const QString Yes;
  const QString No;
  const QString Start;
  const QString End;
  const QString ScriptingName;
  const QString Comment;
  const QString Color;
  const QString Label;
  const QString Extended;
  const QString uuid;
  const QString Description;
  const QString Components;
  const QString Parents;
  const QString Accessors;
  const QString Data;
  const QString Power;
  const QString DefaultDuration;
  const QString MinDuration;
  const QString MaxDuration;
  const QString GuiDuration;
  const QString Rigidity;
  const QString MinNull;
  const QString MaxInf;
  const QString Processes;
  const QString Racks;
  const QString FullView;
  const QString Slots;
  const QString StartState;
  const QString EndState;
  const QString Date;
  const QString StartDate;
  const QString PreviousConstraint;
  const QString NextConstraint;
  const QString Events;
  const QString Extent;
  const QString Active;
  const QString Expression;
  const QString Trigger;
  const QString Event;
  const QString HeightPercentage;
  const QString Messages;
  const QString StateProcesses;
  const QString TimeNode;
  const QString States;
  const QString Condition;
  const QString Offset;
  const QString Segments;
  const QString SmallViewRack;
  const QString FullViewRack;
  const QString Zoom;
  const QString Center;
  const QString SmallViewShown;
  const QString Height;
  const QString Width;
};

ISCORE_LIB_BASE_EXPORT const StringConstants& StringConstant();
}
