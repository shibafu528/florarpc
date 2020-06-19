#include "ProtocolTreeModel.h"
#include <variant>

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

    int32_t index;
    shared_ptr<Node> parent;
    vector<shared_ptr<Node>> children;
    Type type;
    std::variant<const FileDescriptor *, const ServiceDescriptor *, const MethodDescriptor *> descriptor;
    shared_ptr<Protocol> protocol;

    Node(int32_t index, const FileDescriptor *descriptor, shared_ptr<Protocol> protocol)
        : index(index), parent(nullptr), children(), type(FileNode), descriptor(descriptor), protocol(move(protocol)) {}

    Node(int32_t index, const ServiceDescriptor *descriptor, shared_ptr<Node> parent)
        : index(index), parent(move(parent)), children(), type(ServiceNode), descriptor(descriptor) {}

    Node(int32_t index, const MethodDescriptor *descriptor, shared_ptr<Node> parent)
        : index(index), parent(move(parent)), children(), type(MethodNode), descriptor(descriptor) {}

    const FileDescriptor *getFileDescriptor() const { return *std::get_if<const FileDescriptor *>(&descriptor); }

    const ServiceDescriptor *getServiceDescriptor() const {
        return *std::get_if<const ServiceDescriptor *>(&descriptor);
    }

    const MethodDescriptor *getMethodDescriptor() const { return *std::get_if<const MethodDescriptor *>(&descriptor); }

    std::shared_ptr<Protocol> getProtocol() const {
        if (std::holds_alternative<const FileDescriptor *>(descriptor)) {
            return protocol;
        } else {
            return parent->getProtocol();
        }
    }
};

ProtocolTreeModel::ProtocolTreeModel(QObject *parent) : QAbstractItemModel(parent) {}

QModelIndex ProtocolTreeModel::addProtocol(const std::shared_ptr<Protocol> &protocol) {
    beginInsertRows(QModelIndex(), nodes.size(), nodes.size());

    const auto fd = protocol->getFileDescriptor();
    const auto fileNode = std::make_shared<Node>(nodes.size(), fd, protocol);
    nodes.push_back(fileNode);
    for (int32_t sindex = 0; sindex < fd->service_count(); sindex++) {
        auto sd = fd->service(sindex);
        auto serviceNode = std::make_shared<Node>(sindex, sd, fileNode);
        fileNode->children.push_back(serviceNode);
        for (int32_t mindex = 0; mindex < sd->method_count(); mindex++) {
            serviceNode->children.push_back(move(std::make_shared<Node>(mindex, sd->method(mindex), serviceNode)));
        }
    }

    endInsertRows();
    return index(nodes.size() - 1, 0, QModelIndex());
}

void ProtocolTreeModel::remove(const QModelIndex &index) {
    beginRemoveRows(index.parent(), index.row(), index.row());

    auto file = indexToFile(index);
    auto remove = std::remove_if(nodes.begin(), nodes.end(), [file](std::shared_ptr<Node> &node) {
        return node->getFileDescriptor() == file;
    });
    nodes.erase(remove, nodes.end());

    for (auto iter = nodes.begin() + index.row(); iter != nodes.end(); iter++) {
        (*iter)->index--;
    }

    endRemoveRows();
}

void ProtocolTreeModel::clear() {
    beginResetModel();
    nodes.clear();
    endResetModel();
}

QModelIndex ProtocolTreeModel::index(int row, int column, const QModelIndex &parent) const {
    if (column != 0 || (parent.isValid() && parent.column() != 0) || nodes.empty()) {
        return QModelIndex();
    }

    const auto &nodes = parent.isValid() ? indexToNode(parent)->children : this->nodes;
    return createIndex(row, 0, (0 <= row && row < nodes.size()) ? nodes[row].get() : nullptr);
}

QModelIndex ProtocolTreeModel::parent(const QModelIndex &child) const {
    if (!child.isValid()) {
        return QModelIndex();
    }

    const auto node = indexToNode(child);
    if (node != nullptr && node->parent) {
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
                    return QString::fromStdString(node->getMethodDescriptor()->name());
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
    if (node == nullptr || node->type == Node::MethodNode) {
        return QAbstractItemModel::flags(index);
    }

    return Qt::ItemFlag::ItemIsEnabled;
}

const google::protobuf::FileDescriptor *ProtocolTreeModel::indexToFile(const QModelIndex &index) {
    return indexToNode(index)->getFileDescriptor();
}

Method ProtocolTreeModel::indexToMethod(const QModelIndex &index) {
    auto node = indexToNode(index);
    return Method(node->getProtocol(), node->getMethodDescriptor());
}

const ProtocolTreeModel::Node *ProtocolTreeModel::indexToNode(const QModelIndex &index) {
    return static_cast<Node *>(index.internalPointer());
}
