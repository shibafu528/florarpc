#ifndef FLORARPC_QFILEINPUTSTREAM_H
#define FLORARPC_QFILEINPUTSTREAM_H

#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <QFile>

namespace importer {
    class QFileInputStream : public google::protobuf::io::ZeroCopyInputStream {
    public:
        explicit QFileInputStream(std::unique_ptr<QFile> file);

        ~QFileInputStream() override;

        bool Next(const void **data, int *size) override;

        void BackUp(int count) override;

        bool Skip(int count) override;

        int64_t ByteCount() const override;

    private:
        class CopyingInputStream : public google::protobuf::io::CopyingInputStream {
        public:
            explicit CopyingInputStream(QFileInputStream *parent);

            int Read(void *buffer, int size) override;

            int Skip(int count) override;

        private:
            QFileInputStream *parent;
        };

        std::unique_ptr<QFile> file;
        CopyingInputStream inputStream;
        google::protobuf::io::CopyingInputStreamAdaptor inputAdaptor;
    };
}

#endif  // FLORARPC_QFILEINPUTSTREAM_H
