#include "QFileInputStream.h"

namespace importer {
    QFileInputStream::QFileInputStream(std::unique_ptr<QFile> file)
        : file(move(file)), inputStream(this), inputAdaptor(&inputStream) {}

    QFileInputStream::~QFileInputStream() {
        if (file->isOpen()) {
            file->close();
        }
    }

    bool QFileInputStream::Next(const void **data, int *size) { return inputAdaptor.Next(data, size); }

    void QFileInputStream::BackUp(int count) { return inputAdaptor.BackUp(count); }

    bool QFileInputStream::Skip(int count) { return inputAdaptor.Skip(count); }

    long long int QFileInputStream::ByteCount() const { return inputAdaptor.ByteCount(); }

    QFileInputStream::CopyingInputStream::CopyingInputStream(QFileInputStream *parent) : parent(parent) {}

    int QFileInputStream::CopyingInputStream::Read(void *buffer, int size) {
        return parent->file->read((char *) buffer, size);
    }

    int QFileInputStream::CopyingInputStream::Skip(int count) {
        return parent->file->skip(count);
    }
}  // namespace importer