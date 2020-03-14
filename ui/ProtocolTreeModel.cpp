#include "ProtocolTreeModel.h"

using google::protobuf::FileDescriptor;
using google::protobuf::ServiceDescriptor;
using google::protobuf::MethodDescriptor;
using std::vector;
using std::unique_ptr;
using std::shared_ptr;
using std::move;

struct ProtocolTreeModel::DescriptorNode {
    enum class Type {
        Service,
        Method,
    };

    const Type type;

protected:
    inline explicit DescriptorNode(Type type) : type(type) {}
};

struct ProtocolTreeModel::MethodNode : public DescriptorNode {
    uint32_t index;
    const MethodDescriptor *descriptor;
    shared_ptr<ServiceNode> parent;

    MethodNode(shared_ptr<ServiceNode> parent, uint32_t index, const MethodDescriptor *descriptor)
            : DescriptorNode(Type::Method) {
        this->parent = move(parent);
        this->index = index;
        this->descriptor = descriptor;
    }
};

struct ProtocolTreeModel::ServiceNode : public DescriptorNode {
    uint32_t index;
    const ServiceDescriptor *descriptor;
    vector<shared_ptr<MethodNode>> methods;

    ServiceNode(uint32_t index, const ServiceDescriptor *descriptor) : DescriptorNode(Type::Service) {
        this->index = index;
        this->descriptor = descriptor;
    }
};

ProtocolTreeModel::ProtocolTreeModel(QObject *parent) : QAbstractItemModel(parent) {
}

void ProtocolTreeModel::addProtocol(const Protocol &protocol) {
    beginResetModel();

    nodes.clear();
    auto fd = protocol.getFileDescriptor();
    for (uint32_t sindex = 0; sindex < fd->service_count(); sindex++) {
        auto sd = fd->service(sindex);
        auto sn = std::make_shared<ServiceNode>(sindex, sd);
        nodes.push_back(sn);
        for (uint32_t mindex = 0; mindex < sd->method_count(); mindex++) {
            sn->methods.push_back(move(std::make_shared<MethodNode>(sn, mindex, sd->method(mindex))));
        }
    }

    endResetModel();
}

QModelIndex ProtocolTreeModel::index(int row, int column, const QModelIndex &parent) const {
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

QModelIndex ProtocolTreeModel::parent(const QModelIndex &child) const {
    if (!child.isValid()) {
        return QModelIndex();
    }

    if (auto method = indexToMethod(child)) {
        return createIndex(method->parent->index, 0, method->parent.get());
    }

    return QModelIndex();
}

int ProtocolTreeModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        if (auto service = indexToService(parent)) {
            return service->methods.size();
        } else {
            return 0;
        }
    } else {
        return nodes.size();
    }
}

int ProtocolTreeModel::columnCount(const QModelIndex &parent) const {
    return 1;
}

QVariant ProtocolTreeModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }

    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if (auto service = indexToService(index)) {
                return QString::fromStdString(service->descriptor->full_name());
            } else if (auto method = indexToMethod(index)) {
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

Qt::ItemFlags ProtocolTreeModel::flags(const QModelIndex &index) const {
    if (!index.isValid()) {
        return QAbstractItemModel::flags(index);
    }

    if (auto method = indexToMethod(index)) {
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

const ServiceDescriptor* ProtocolTreeModel::indexToServiceDescriptor(const QModelIndex &index) {
    return indexToService(index)->descriptor;
}

const MethodDescriptor* ProtocolTreeModel::indexToMethodDescriptor(const QModelIndex &index) {
    return indexToMethod(index)->descriptor;
}

const ProtocolTreeModel::ServiceNode* ProtocolTreeModel::indexToService(const QModelIndex &index) {
    auto node = static_cast<DescriptorNode*>(index.internalPointer());
    if (node->type == DescriptorNode::Type::Service) {
        return reinterpret_cast<ServiceNode*>(node);
    }
    return nullptr;
}

const ProtocolTreeModel::MethodNode* ProtocolTreeModel::indexToMethod(const QModelIndex &index) {
    auto node = static_cast<DescriptorNode*>(index.internalPointer());
    if (node->type == DescriptorNode::Type::Method) {
        return reinterpret_cast<MethodNode*>(node);
    }
    return nullptr;
}
