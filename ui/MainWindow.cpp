#include "MainWindow.h"

#include <google/protobuf/util/json_util.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QScreen>
#include <QStandardPaths>
#include <QStyle>
#include <QTextStream>

#include "AboutDialog.h"
#include "ImportsManageDialog.h"
#include "ServersManageDialog.h"
#include "flora_constants.h"
#include "florarpc/workspace.pb.h"
#include "util/ProtobufIterator.h"

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent), protocolTreeModel(std::make_unique<ProtocolTreeModel>(this)),
          tabCloseShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this),
          treeFileContextMenu(this),
          treeMethodContextMenu(this) {
    ui.setupUi(this);

    connect(ui.actionOpen, &QAction::triggered, this, &MainWindow::onActionOpenTriggered);
    connect(ui.actionOpenWorkspace, &QAction::triggered, this, &MainWindow::onActionOpenWorkspaceTriggered);
    connect(ui.actionSaveWorkspace, &QAction::triggered, this, &MainWindow::onActionSaveWorkspaceTriggered);
    connect(ui.actionManageProto, &QAction::triggered, this, &MainWindow::onActionManageProtoTriggered);
    connect(ui.actionManageServer, &QAction::triggered, this, &MainWindow::onActionManageServerTriggered);
    connect(ui.actionAbout, &QAction::triggered, [=]() {
        AboutDialog dialog;
        dialog.exec();
    });
    connect(ui.actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(ui.treeView, &QTreeView::clicked, this, &MainWindow::onTreeViewClicked);
    connect(ui.treeView, &QWidget::customContextMenuRequested, [=](const QPoint &pos) {
        const QModelIndex &index = ui.treeView->indexAt(pos);
        if (!index.parent().isValid()) {
            // file node
            treeFileContextMenu.exec(ui.treeView->viewport()->mapToGlobal(pos));
        } else if (index.parent().isValid() && index.flags().testFlag(Qt::ItemFlag::ItemIsSelectable)) {
            // method node
            treeMethodContextMenu.exec(ui.treeView->viewport()->mapToGlobal(pos));
        }
    });
    connect(ui.editorTabs, &QTabWidget::tabCloseRequested, this, &MainWindow::onEditorTabCloseRequested);
    connect(&tabCloseShortcut, &QShortcut::activated, this, &MainWindow::onTabCloseShortcutActivated);

    auto toggleLogViewAction = ui.logDockWidget->toggleViewAction();
    toggleLogViewAction->setShortcut(Qt::CTRL + Qt::Key_L);
    ui.menuView->addAction(toggleLogViewAction);

    ui.logDockWidget->hide();
    ui.treeView->setModel(protocolTreeModel.get());

    treeFileContextMenu.addAction("ワークスペースから削除(&D)", this, &MainWindow::onRemoveFileFromTreeTriggered);

    treeMethodContextMenu.addAction("開く(&O)", [=]() {
        const QModelIndex &index =
            ui.treeView->indexAt(ui.treeView->viewport()->mapFromGlobal(treeMethodContextMenu.pos()));
        openMethod(index, false);
    });
    treeMethodContextMenu.addAction("新しいタブで開く(&N)", [=]() {
        const QModelIndex &index =
            ui.treeView->indexAt(ui.treeView->viewport()->mapFromGlobal(treeMethodContextMenu.pos()));
        openMethod(index, true);
    });

    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(),
                                    QGuiApplication::primaryScreen()->availableGeometry()));
    setWindowTitle(QString("%1 - FloraRPC").arg("新しいワークスペース"));
}

void MainWindow::onLogging(const QString &message) { ui.logEdit->appendPlainText(message); }

void MainWindow::onActionOpenTriggered() {
    auto filenames = QFileDialog::getOpenFileNames(this, "Open proto", "", "Proto definition files (*.proto)", nullptr);
    openProtos(filenames, true);
}

void MainWindow::onActionOpenWorkspaceTriggered() {
    QString baseDirectory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if (!workspaceFilename.isEmpty()) {
        baseDirectory = QFileInfo(workspaceFilename).dir().path();
    }
    auto filename =
        QFileDialog::getOpenFileName(this, "Open workspace", baseDirectory, "FloraRPC Workspace (*.floraws)");
    if (filename.isEmpty()) {
        return;
    }

    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::critical(this, "Open error",
                              "ワークスペースの読み込み中にエラーが発生しました。\nファイルを開けません。");
        return;
    }

    const QByteArray workspaceBin = file.readAll();
    file.close();

    florarpc::Workspace workspace;
    bool success = workspace.ParseFromString(workspaceBin.toStdString());
    if (!success) {
        QMessageBox::critical(
            this, "Open error",
            "ワークスペースの読み込み中にエラーが発生しました。\nファイルを読み込むことができません。");
        return;
    }

    if (workspace.app_version().major() > FLORA_VERSION_MAJOR ||
        workspace.app_version().minor() > FLORA_VERSION_MINOR ||
        workspace.app_version().patch() > FLORA_VERSION_PATCH ||
        workspace.app_version().tweak() > FLORA_VERSION_TWEAK) {
        QMessageBox::warning(this, "Open error",
                             "このワークスペースは現在実行中のFloraRPCよりも新しいバージョンで保存されています。\n読み"
                             "込みを中止します。");
        return;
    }

    protocols.clear();
    imports.clear();
    servers.clear();
    certificates.clear();
    for (int i = ui.editorTabs->count() - 1; i >= 0; i--) {
        auto editor = ui.editorTabs->widget(i);
        ui.editorTabs->removeTab(i);
        delete editor;
    }
    protocolTreeModel->clear();

    for (const auto &importPath : workspace.import_paths()) {
        imports.append(QString::fromStdString(importPath.path()));
    }

    for (const auto &server : workspace.servers()) {
        servers.push_back(std::make_shared<Server>(server));
    }

    for (const auto &certificate : workspace.certificates()) {
        certificates.push_back(std::make_shared<Certificate>(certificate));
    }

    QStringList filenames;
    for (const auto &protoFile : workspace.proto_files()) {
        filenames << QString::fromStdString(protoFile.path());
    }
    openProtos(filenames, false);

    for (const auto &request : workspace.requests()) {
        const auto &methodRef = request.method();
        for (const auto &protocol : protocols) {
            if (protocol->getSourceAbsolutePath() != methodRef.file_name()) {
                continue;
            }

            auto method = protocol->findMethodByRef(methodRef);
            if (method != nullptr) {
                auto editor = openEditor(std::make_unique<Method>(protocol, method), true);
                editor->readRequest(request);
                break;
            }
        }
    }

    ui.statusbar->showMessage("ワークスペースを読み込みました", 5000);
    setWorkspaceFilename(filename);
}

void MainWindow::onActionSaveWorkspaceTriggered() {
    QString baseDirectory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if (!workspaceFilename.isEmpty()) {
        baseDirectory = QFileInfo(workspaceFilename).dir().path();
    }
    auto filename =
        QFileDialog::getSaveFileName(this, "Save workspace", baseDirectory, "FloraRPC Workspace (*.floraws)");
    if (filename.isEmpty()) {
        return;
    }

    if (saveWorkspace(filename)) {
        ui.statusbar->showMessage("ワークスペースを保存しました", 5000);
        setWorkspaceFilename(filename);
    }
}

void MainWindow::onActionManageProtoTriggered() {
    auto dialog = std::make_unique<ImportsManageDialog>(this);
    dialog->setPaths(imports);
    dialog->exec();
    imports = dialog->getPaths();
}

void MainWindow::onActionManageServerTriggered() {
    auto dialog = std::make_unique<ServersManageDialog>(this);
    dialog->setServers(servers);
    dialog->setCertificates(certificates);
    dialog->exec();
    servers = dialog->getServers();
    certificates = dialog->getCertificates();
    for (int i = 0; i < ui.editorTabs->count(); i++) {
        auto editor = qobject_cast<Editor *>(ui.editorTabs->widget(i));
        if (editor != nullptr) {
            editor->setServers(servers);
            editor->setCertificates(certificates);
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (workspaceFilename.isEmpty()) {
        QMessageBox messageBox;
        messageBox.setIcon(QMessageBox::Warning);
        messageBox.setWindowTitle("確認");
        messageBox.setText("ワークスペースは保存されていません。\n保存してからアプリケーションを終了しますか？");
        messageBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        messageBox.setDefaultButton(QMessageBox::Save);
        auto response = messageBox.exec();
        if (response == QMessageBox::Save) {
            auto filename = QFileDialog::getSaveFileName(this, "Save workspace", "", "FloraRPC Workspace (*.floraws)");
            if (filename.isEmpty()) {
                event->ignore();
                return;
            }
            if (!saveWorkspace(filename)) {
                event->ignore();
            }
        } else if (response == QMessageBox::Cancel) {
            event->ignore();
        }
    } else {
        if (saveWorkspace(workspaceFilename)) {
            return;
        }

        if (QMessageBox::warning(this, "確認",
                                 "ワークスペースは保存されていません！\nアプリケーションを終了してもよろしいですか？",
                                 QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
            event->ignore();
        }
    }
}

void MainWindow::onTreeViewClicked(const QModelIndex &index) { openMethod(index, false); }

void MainWindow::onRemoveFileFromTreeTriggered() {
    const QModelIndex &index = ui.treeView->indexAt(ui.treeView->viewport()->mapFromGlobal(treeFileContextMenu.pos()));
    if (index.parent().isValid()) {
        return;
    }

    if (QMessageBox::warning(this, "ワークスペースから削除",
                             "このファイルに含まれるメソッドのタブは閉じられますが、よろしいですか？",
                             QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok) {
        auto fileDescriptor = ProtocolTreeModel::indexToFile(index);

        for (int i = ui.editorTabs->count() - 1; i >= 0; i--) {
            auto editor = qobject_cast<Editor *>(ui.editorTabs->widget(i));
            if (editor->getMethod().isChildOf(fileDescriptor)) {
                ui.editorTabs->removeTab(i);
                delete editor;
            }
        }

        protocolTreeModel->remove(index);

        auto remove = std::remove_if(protocols.begin(), protocols.end(), [=](std::shared_ptr<Protocol> &p) {
            return p->getFileDescriptor() == fileDescriptor;
        });
        protocols.erase(remove, protocols.end());
    }
}

void MainWindow::onEditorTabCloseRequested(const int index) {
    auto editor = ui.editorTabs->widget(index);
    ui.editorTabs->removeTab(index);
    delete editor;
}

void MainWindow::onTabCloseShortcutActivated() {
    auto editor = ui.editorTabs->currentWidget();
    if (editor) {
        ui.editorTabs->removeTab(ui.editorTabs->currentIndex());
        delete editor;
    }
}

void MainWindow::openProtos(const QStringList &filenames, bool abortOnLoadError) {
    if (filenames.isEmpty()) {
        return;
    }
    std::vector<std::shared_ptr<Protocol>> successes;
    for (const auto &filename : filenames) {
        QFileInfo file(filename);

        if (std::any_of(protocols.begin(), protocols.end(),
                        [file](std::shared_ptr<Protocol> &p) { return p->getSource() == file; })) {
            if (filenames.size() == 1) {
                QMessageBox::warning(this, "Load error", "このファイルはすでに読み込まれています。");
            }
            continue;
        }

        try {
            const auto protocol = std::make_shared<Protocol>(file, imports);
            successes.push_back(protocol);
        } catch (ProtocolLoadException &e) {
            QString message = "Protoファイルの読込中にエラーが発生しました。\n";
            QTextStream stream(&message);

            for (const auto &err : *e.errors) {
                if (&err != &e.errors->front()) {
                    stream << '\n';
                }
                stream << QString::fromStdString(err);
            }
            QMessageBox::critical(this, "Load error", message);
            if (abortOnLoadError) {
                return;
            }
        }
    }
    for (const auto &protocol : successes) {
        protocols.push_back(protocol);
        const auto index = protocolTreeModel->addProtocol(protocol);
        ui.treeView->expandRecursively(index);
    }
}

void MainWindow::openMethod(const QModelIndex &index, bool forceNewTab) {
    if (!index.parent().isValid() || !index.flags().testFlag(Qt::ItemFlag::ItemIsSelectable)) {
        // disabled node
        return;
    }

    openEditor(std::make_unique<Method>(ProtocolTreeModel::indexToMethod(index)), forceNewTab);
}

Editor *MainWindow::openEditor(std::unique_ptr<Method> method, bool forceNewTab) {
    auto methodName = method->getFullName();

    // Find exists tab
    if (!forceNewTab) {
        for (int i = 0; i < ui.editorTabs->count(); i++) {
            auto editor = qobject_cast<Editor *>(ui.editorTabs->widget(i));
            if (editor != nullptr && editor->getMethod().getFullName() == methodName) {
                ui.editorTabs->setCurrentIndex(ui.editorTabs->indexOf(editor));
                return editor;
            }
        }
    }

    auto editor = new Editor(std::move(method), syntaxDefinitions);
    editor->setServers(servers);
    editor->setCertificates(certificates);
    const auto addedIndex = ui.editorTabs->addTab(editor, QString::fromStdString(methodName));
    ui.editorTabs->setCurrentIndex(addedIndex);
    return editor;
}

bool MainWindow::saveWorkspace(const QString &filename) {
    florarpc::Workspace workspace;
    florarpc::Version *version = workspace.mutable_app_version();
    version->set_major(FLORA_VERSION_MAJOR);
    version->set_minor(FLORA_VERSION_MINOR);
    version->set_patch(FLORA_VERSION_PATCH);
    version->set_tweak(FLORA_VERSION_TWEAK);

    for (auto &protocol : protocols) {
        florarpc::ProtoFile *file = workspace.add_proto_files();
        file->set_path(protocol->getSource().absoluteFilePath().toStdString());
    }

    for (auto &path : imports) {
        florarpc::ImportPath *importPath = workspace.add_import_paths();
        importPath->set_path(path.toStdString());
    }

    for (auto &server : servers) {
        florarpc::Server *protoServer = workspace.add_servers();
        server->writeServer(*protoServer);
    }

    for (auto &certificate : certificates) {
        florarpc::Certificate *protoCertificate = workspace.add_certificates();
        certificate->writeCertificate(*protoCertificate);
    }

    for (int i = 0; i < ui.editorTabs->count(); i++) {
        auto editor = qobject_cast<Editor *>(ui.editorTabs->widget(i));
        if (editor != nullptr) {
            florarpc::Request *request = workspace.add_requests();
            editor->writeRequest(*request);
        }
    }

    std::string output;
    if (!workspace.SerializeToString(&output)) {
        QMessageBox::critical(
            this, "Save error",
            "ワークスペースの保存中にエラーが発生しました。\nワークスペース情報の書き出し準備に失敗しました。");
        return false;
    }

    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox::critical(this, "Save error",
                              "ワークスペースの保存中にエラーが発生しました。\n保存先のファイルを作成できません。");
        return false;
    }
    file.write(QByteArray::fromStdString(output));
    file.close();

    return true;
}

void MainWindow::setWorkspaceFilename(const QString &filename) {
    workspaceFilename = filename;
    QFileInfo fileInfo(filename);
    setWindowTitle(QString("%1 - FloraRPC").arg(fileInfo.baseName()));
}
