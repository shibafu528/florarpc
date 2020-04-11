#ifndef FLORARPC_PROTOBUFITERATOR_H
#define FLORARPC_PROTOBUFITERATOR_H

#include <google/protobuf/descriptor.h>

namespace ProtobufIterator {
    using namespace google::protobuf;

    template<class T>
    struct Adapter {
    };

    template<>
    struct Adapter<FieldDescriptor> {
        static inline const FieldDescriptor *lookup(const Descriptor *d, int i) {
            return d->field(i);
        }

        static inline int count(const Descriptor *d) {
            return d->field_count();
        }
    };

    template<class T>
    class Iterator {
    public:
        Iterator(const Descriptor *d, int index) : d(d), index(index) {}

        const T *operator*() {
            return Adapter<T>::lookup(d, index);
        }

        const T *operator->() {
            return Adapter<T>::lookup(d, index);
        }

        Iterator<T> &operator++() {
            index++;
            return *this;
        }

        Iterator<T> operator++(int) {
            Iterator<T> res = *this;
            index++;
            return res;
        }

        bool operator!=(const Iterator<T> &iter) {
            return this->d != iter.d || this->index != iter.index;
        }

    private:
        const Descriptor *d;
        int index;
    };

    template<class T>
    class Iterable {
    public:
        Iterable(const Descriptor *d) : d(d) {}

        Iterator<T> begin() {
            return Iterator<T>(d, 0);
        }

        Iterator<T> end() {
            return Iterator<T>(d, Adapter<T>::count(d));
        }

    private:
        const Descriptor *d;
    };
}

#endif //FLORARPC_PROTOBUFITERATOR_H
