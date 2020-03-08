#ifndef FLORARPC_PROTOCOLMODEL_H
#define FLORARPC_PROTOCOLMODEL_H

#include <QAbstractItemModel>
#include "../entity/Protocol.h"

struct DescriptorNode;
struct MethodNode;
struct ServiceNode;

struct DescriptorNode {
    enum class Type {
        Service,
        Method,
    };

    const Type type;

protected:
    inline explicit DescriptorNode(Type type) : type(type) {}
};

struct MethodNode : public DescriptorNode {
    uint32_t index;
    const google::protobuf::MethodDescriptor *descriptor;
    std::shared_ptr<ServiceNode> parent;

    MethodNode(std::shared_ptr<ServiceNode> parent, uint32_t index, const google::protobuf::MethodDescriptor *descriptor);
};

struct ServiceNode : public DescriptorNode {
    uint32_t index;
    const google::protobuf::ServiceDescriptor *descriptor;
    std::vector<std::shared_ptr<MethodNode>> methods;

    ServiceNode(uint32_t index, const google::protobuf::ServiceDescriptor *descriptor);
};

class ProtocolModel : public QAbstractItemModel {
public:
    ProtocolModel(QObject *parent, Protocol *protocol);

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;

    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent) const override;

    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    Protocol *protocol;
    std::vector<std::shared_ptr<ServiceNode>> nodes;
};


#endif //FLORARPC_PROTOCOLMODEL_H
