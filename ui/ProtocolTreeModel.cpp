#include "ProtocolTreeModel.h"

using google::protobuf::FileDescriptor;
using google::protobuf::ServiceDescriptor;
using google::protobuf::MethodDescriptor;
using std::vector;
using std::unique_ptr;
using std::shared_ptr;
using std::move;

struct ProtocolTreeModel::Node {
    enum Type {
        FileNode,
        ServiceNode,
        MethodNode,
    };

    uint32_t index;
    shared_ptr<Node> parent;
    vector<shared_ptr<Node>> children;
    Type type;
    union {
        const FileDescriptor* file;
        const ServiceDescriptor* service;
        const MethodDescriptor* method;
    } const descriptor;

    Node(uint32_t index, const FileDescriptor *descriptor)
            : index(index), parent(nullptr), children(), type(FileNode), descriptor({.file = descriptor}) {}

    Node(uint32_t index, const ServiceDescriptor *descriptor, shared_ptr<Node> parent)
            : index(index), parent(move(parent)), children(), type(ServiceNode), descriptor({.service = descriptor}) {}

    Node(uint32_t index, const MethodDescriptor *descriptor, shared_ptr<Node> parent)
            : index(index), parent(move(parent)), children(), type(MethodNode), descriptor({.method = descriptor}) {}

    const FileDescriptor* getFileDescriptor() const {
        return type == FileNode ? descriptor.file : nullptr;
    }

    const ServiceDescriptor* getServiceDescriptor() const {
        return type == ServiceNode ? descriptor.service : nullptr;
    }

    const MethodDescriptor* getMethodDescriptor() const {
        return type == MethodNode ? descriptor.method : nullptr;
    }
};

ProtocolTreeModel::ProtocolTreeModel(QObject *parent) : QAbstractItemModel(parent) {}

void ProtocolTreeModel::addProtocol(const Protocol &protocol) {
    // TODO: Resetじゃなくてもよさそう
    beginResetModel();

    const auto fd = protocol.getFileDescriptor();
    const auto fileNode = std::make_shared<Node>(nodes.size(), fd);
    nodes.push_back(fileNode);
    for (uint32_t sindex = 0; sindex < fd->service_count(); sindex++) {
        auto sd = fd->service(sindex);
        auto serviceNode = std::make_shared<Node>(sindex, sd, fileNode);
        fileNode->children.push_back(serviceNode);
        for (uint32_t mindex = 0; mindex < sd->method_count(); mindex++) {
            serviceNode->children.push_back(move(std::make_shared<Node>(mindex, sd->method(mindex), serviceNode)));
        }
    }

    endResetModel();
}

QModelIndex ProtocolTreeModel::index(int row, int column, const QModelIndex &parent) const {
    if (column != 0 || (parent.isValid() && parent.column() != 0)) {
        return QModelIndex();
    }

    if (parent.isValid()) {
        const auto parentNode = indexToNode(parent);
        return createIndex(row, 0, parentNode->children[row].get());
    } else {
        return createIndex(row, 0, nodes[row].get());
    }
}

QModelIndex ProtocolTreeModel::parent(const QModelIndex &child) const {
    if (!child.isValid()) {
        return QModelIndex();
    }

    const auto node = indexToNode(child);
    if (node->parent) {
        return createIndex(node->parent->index, 0, node->parent.get());
    }

    return QModelIndex();
}

int ProtocolTreeModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        const auto node = indexToNode(parent);
        return node->children.size();
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

    const auto node = indexToNode(index);
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            switch (node->type) {
                case Node::FileNode:
                    return QString::fromStdString(node->getFileDescriptor()->name());
                case Node::ServiceNode:
                    return QString::fromStdString(node->getServiceDescriptor()->full_name());
                case Node::MethodNode: {
                    const auto descriptor = node->getMethodDescriptor();
                    if (role == Qt::ToolTipRole && (descriptor->client_streaming() || descriptor->server_streaming())) {
                        return QString::fromStdString(descriptor->name()) + "<hr><b>Streaming RPC is not supported yet.</b>";
                    }
                    return QString::fromStdString(descriptor->name());
                }
            }
            break;
        default:
            return QVariant();
    }

    return QVariant();
}

Qt::ItemFlags ProtocolTreeModel::flags(const QModelIndex &index) const {
    if (!index.isValid()) {
        return QAbstractItemModel::flags(index);
    }

    const auto node = indexToNode(index);
    if (node->type == Node::MethodNode) {
        const auto descriptor = node->getMethodDescriptor();
        if (descriptor->client_streaming() || descriptor->server_streaming()) {
            // Streaming RPC is not supported yet
            return Qt::ItemFlag::NoItemFlags;
        }

        return QAbstractItemModel::flags(index);
    }

    return Qt::ItemFlag::ItemIsEnabled;
}

const ServiceDescriptor* ProtocolTreeModel::indexToServiceDescriptor(const QModelIndex &index) {
    return indexToNode(index)->getServiceDescriptor();
}

const MethodDescriptor* ProtocolTreeModel::indexToMethodDescriptor(const QModelIndex &index) {
    return indexToNode(index)->getMethodDescriptor();
}

const ProtocolTreeModel::Node* ProtocolTreeModel::indexToNode(const QModelIndex &index) {
    return static_cast<Node*>(index.internalPointer());
}
