#ifndef FLORARPC_WORKSPACEMODIFIEDEVENT_H
#define FLORARPC_WORKSPACEMODIFIEDEVENT_H

#include <QEvent>
#include <QString>

namespace Event {
    class WorkspaceModifiedEvent : public QEvent {
    public:
        WorkspaceModifiedEvent(const QString &sender);

        const QString sender;
        static const QEvent::Type type;
    };
}  // namespace Event

#endif  // FLORARPC_WORKSPACEMODIFIEDEVENT_H
