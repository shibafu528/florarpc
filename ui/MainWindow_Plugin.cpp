#include <QClipboard>
#include <QFileDialog>
#include <QJSEngine>
#include <QTextStream>

#include "MainWindow.h"

void MainWindow::onActionCopyAsGrpcurlTriggered() {
    const auto editor = qobject_cast<Editor *>(ui.editorTabs->currentWidget());
    if (editor == nullptr) {
        return;
    }

    std::unique_ptr<Request> request(editor->makeRequest());
    if (request == nullptr) {
        return;
    }

    QJSEngine js;
    js.installExtensions(QJSEngine::ConsoleExtension | QJSEngine::GarbageCollectionExtension);
    {
        QJSValue req = js.newObject();
        req.setProperty("body", request->getBody());

        QJSValue meta = js.newObject();
        for (auto iter = request->getMetadata().cbegin(); iter != request->getMetadata().cend(); ++iter) {
            meta.setProperty(iter.key(), iter.value());
        }
        req.setProperty("metadata", meta);

        for (int i = 0; i < protocolTreeModel->rowCount(QModelIndex()); i++) {
            const auto index = protocolTreeModel->index(i, 0, QModelIndex());
            const auto fileDescriptor = protocolTreeModel->indexToFile(index);
            if (request->getMethod().isChildOf(fileDescriptor)) {
                const auto fileInfo = protocolTreeModel->indexToSourceFile(index);
                req.setProperty("protoFile", fileInfo.absoluteFilePath());
                break;
            }
        }

        req.setProperty("path", QString::fromStdString(request->getMethod().getRequestPath()));

        js.globalObject().setProperty("request", req);
    }
    {
        QJSValue imp = js.newArray(imports.size());
        for (int i = 0; i < imports.size(); i++) {
            imp.setProperty(i, imports[i]);
        }
        js.globalObject().setProperty("imports", imp);
    }
    if (const auto &server = request->getServer(); server) {
        QJSValue svr = js.newObject();

        svr.setProperty("address", server->address);
        svr.setProperty("useTLS", server->useTLS);
        if (server->useTLS) {
            auto certificate = server->findCertificate(certificates);
            if (certificate) {
                QJSValue cert = js.newObject();
                cert.setProperty("rootCerts", certificate->rootCertsPath);
                cert.setProperty("privateKey", certificate->privateKeyPath);
                cert.setProperty("certChain", certificate->certChainPath);
                svr.setProperty("certificate", cert);
            }
        }

        js.globalObject().setProperty("server", svr);
    }

    // TODO: tmp
    QString fileName = QFileDialog::getOpenFileName(this, QString(), QString(), "JavaScript file (*.js)");
    QFile f(fileName);
    f.open(QFile::ReadOnly);
    QJSValue ret = js.evaluate(f.readAll());
    if (ret.isError()) {
        ui.statusbar->showMessage(
            QString("Error at %1: %2").arg(ret.property("lineNumber").toInt()).arg(ret.toString()), 10000);
    } else if (ret.isString()) {
        if (auto str = ret.toString(); !str.isEmpty()) {
            QApplication::clipboard()->setText(str);
            ui.statusbar->showMessage("Copied!", 5000);
        } else {
            ui.statusbar->showMessage("Result is empty.", 5000);
        }
    } else {
        ui.statusbar->showMessage("Result is not string.", 5000);
    }
}
