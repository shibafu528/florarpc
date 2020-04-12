#ifndef FLORARPC_PROTOBUFITERATOR_H
#define FLORARPC_PROTOBUFITERATOR_H

#include <google/protobuf/descriptor.h>

namespace ProtobufIterator {
    using namespace google::protobuf;

    template <class Parent, class Child>
    struct Adapter {};

    template <>
    struct Adapter<Descriptor, FieldDescriptor> {
        static inline const FieldDescriptor *lookup(const Descriptor *d, int i) { return d->field(i); }

        static inline int count(const Descriptor *d) { return d->field_count(); }
    };

    template <>
    struct Adapter<const FileDescriptor, const ServiceDescriptor> {
        static inline const ServiceDescriptor *lookup(const FileDescriptor *d, int i) { return d->service(i); }

        static inline int count(const FileDescriptor *d) { return d->service_count(); }
    };

    template <>
    struct Adapter<const ServiceDescriptor, const MethodDescriptor> {
        static inline const MethodDescriptor *lookup(const ServiceDescriptor *d, int i) { return d->method(i); }

        static inline int count(const ServiceDescriptor *d) { return d->method_count(); }
    };

    template <class Parent, class Child>
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Child;
        using pointer = Child *;
        using reference = Child &;
        using difference_type = std::ptrdiff_t;

        Iterator(const Parent *d, int index) : d(d), index(index) {}

        const Child *operator*() { return Adapter<Parent, Child>::lookup(d, index); }

        const Child *operator->() { return Adapter<Parent, Child>::lookup(d, index); }

        Iterator<Parent, Child> &operator++() {
            index++;
            return *this;
        }

        Iterator<Parent, Child> operator++(int) {
            Iterator<Parent, Child> res = *this;
            index++;
            return res;
        }

        bool operator==(const Iterator<Parent, Child> &iter) { return this->d == iter.d && this->index == iter.index; }

        bool operator!=(const Iterator<Parent, Child> &iter) { return this->d != iter.d || this->index != iter.index; }

    private:
        const Parent *d;
        int index;
    };

    template <class Parent, class Child>
    class Iterable {
    public:
        Iterable(const Parent *d) : d(d) {}

        Iterator<Parent, Child> begin() { return Iterator<Parent, Child>(d, 0); }

        Iterator<Parent, Child> end() { return Iterator<Parent, Child>(d, Adapter<Parent, Child>::count(d)); }

    private:
        const Parent *d;
    };
}  // namespace ProtobufIterator

#endif  // FLORARPC_PROTOBUFITERATOR_H
