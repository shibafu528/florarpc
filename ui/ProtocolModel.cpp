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
        auto sn = std::make_shared<ServiceNode>(sindex, sd);
        nodes.push_back(sn);
        for (uint32_t mindex = 0; mindex < sd->method_count(); mindex++) {
            sn->methods.push_back(move(std::make_shared<MethodNode>(sn, mindex, sd->method(mindex))));
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
    if (!index.isValid()) {
        return QVariant();
    }

    auto node = static_cast<DescriptorNode *>(index.internalPointer());

    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if (node->type == DescriptorNode::Type::Service) {
                auto service = reinterpret_cast<ServiceNode *>(node);
                return QString::fromStdString(service->descriptor->full_name());
            } else {
                auto method = reinterpret_cast<MethodNode *>(node);
                if (role == Qt::ToolTipRole && (method->descriptor->client_streaming() || method->descriptor->server_streaming())) {
                    return QString::fromStdString(method->descriptor->name()) + "<hr><b>Streaming RPC is not supported yet.</b>";
                }
                return QString::fromStdString(method->descriptor->name());
            }
        default:
            return QVariant();
    }

    return QVariant();
}

Qt::ItemFlags ProtocolModel::flags(const QModelIndex &index) const {
    if (!index.isValid()) {
        return QAbstractItemModel::flags(index);
    }

    if (index.parent().isValid()) {
        // is Method
        auto method = static_cast<MethodNode *>(index.internalPointer());
        if (method->descriptor->client_streaming() || method->descriptor->server_streaming()) {
            // Streaming RPC is not supported yet
            return Qt::ItemFlag::NoItemFlags;
        }

        return QAbstractItemModel::flags(index);
    } else {
        // is Service
        return Qt::ItemFlag::ItemIsEnabled;
    }
}
