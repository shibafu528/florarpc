#include "WorkspaceModifiedEvent.h"

Event::WorkspaceModifiedEvent::WorkspaceModifiedEvent(const QString &sender) : QEvent(type), sender(sender) {}

const QEvent::Type Event::WorkspaceModifiedEvent::type = static_cast<QEvent::Type>(QEvent::User + 1);
