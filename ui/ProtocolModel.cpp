#include "ProtocolModel.h"

using google::protobuf::FileDescriptor;
using google::protobuf::ServiceDescriptor;
using google::protobuf::MethodDescriptor;
using std::vector;
using std::unique_ptr;
using std::shared_ptr;
using std::move;

MethodNode::MethodNode(shared_ptr<ServiceNode> parent, uint32_t index, const MethodDescriptor *descriptor)
        : DescriptorNode(Type::Method) {
    this->parent = move(parent);
    this->index = index;
    this->descriptor = descriptor;
}

ServiceNode::ServiceNode(uint32_t index, const ServiceDescriptor *descriptor)
        : DescriptorNode(Type::Service) {
    this->index = index;
    this->descriptor = descriptor;
}

ProtocolModel::ProtocolModel(QObject *parent, Protocol *protocol)
        : QAbstractItemModel(parent), protocol(protocol) {
    auto fd = protocol->getFileDescriptor();
    for (uint32_t sindex = 0; sindex < fd->service_count(); sindex++) {
        auto sd = fd->service(sindex);
        shared_ptr<ServiceNode> sn(new ServiceNode(sindex, sd));
        nodes.push_back(sn);
        for (uint32_t mindex = 0; mindex < sd->method_count(); mindex++) {
            shared_ptr<MethodNode> mn(new MethodNode(sn, mindex, sd->method(mindex)));
            sn->methods.push_back(move(mn));
        }
    }
}

QModelIndex ProtocolModel::index(int row, int column, const QModelIndex &parent) const {
    if (column != 0 || (parent.isValid() && parent.column() != 0)) {
        return QModelIndex();
    }

    if (parent.isValid()) {
        auto service = static_cast<ServiceNode *>(parent.internalPointer());
        return createIndex(row, 0, (void *) service->methods[row].get());
    } else {
        return createIndex(row, 0, (void *) nodes[row].get());
    }
}

QModelIndex ProtocolModel::parent(const QModelIndex &child) const {
    if (!child.isValid()) {
        return QModelIndex();
    }

    auto node = static_cast<DescriptorNode *>(child.internalPointer());
    if (node->type == DescriptorNode::Type::Method) {
        auto method = reinterpret_cast<MethodNode *>(node);
        return createIndex(method->parent->index, 0, method->parent.get());
    }

    return QModelIndex();
}

int ProtocolModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        auto node = static_cast<DescriptorNode *>(parent.internalPointer());
        if (node->type == DescriptorNode::Type::Method) {
            return 0;
        } else {
            return reinterpret_cast<ServiceNode *>(node)->methods.size();
        }
    } else {
        return nodes.size();
    }
}

int ProtocolModel::columnCount(const QModelIndex &parent) const {
    return 1;
}

QVariant ProtocolModel::data(const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole || !index.isValid()) {
        return QVariant();
    }

    if (index.parent().isValid()) {
        // is Method
        auto method = static_cast<MethodNode *>(index.internalPointer());
        return QString::fromStdString(method->descriptor->name());
    } else {
        // is Service
        auto service = static_cast<ServiceNode *>(index.internalPointer());
        return QString::fromStdString(service->descriptor->full_name());
    }
}

Qt::ItemFlags ProtocolModel::flags(const QModelIndex &index) const {
    if (index.isValid() && !index.parent().isValid()) {
        // is Service
        return Qt::ItemFlag::ItemIsEnabled;
    }

    return QAbstractItemModel::flags(index);
}
