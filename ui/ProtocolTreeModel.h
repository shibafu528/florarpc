#ifndef FLORARPC_PROTOCOLTREEMODEL_H
#define FLORARPC_PROTOCOLTREEMODEL_H

#include <QAbstractItemModel>
#include "../entity/Protocol.h"

class ProtocolTreeModel : public QAbstractItemModel {
public:
    ProtocolTreeModel(QObject *parent);

    QModelIndex addProtocol(const Protocol &protocol);

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;

    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent) const override;

    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    static const google::protobuf::ServiceDescriptor* indexToServiceDescriptor(const QModelIndex &index);

    static const google::protobuf::MethodDescriptor* indexToMethodDescriptor(const QModelIndex &index);

private:
    struct Node;

    std::vector<std::shared_ptr<Node>> nodes;

    static const Node* indexToNode(const QModelIndex &index);
};


#endif //FLORARPC_PROTOCOLTREEMODEL_H
