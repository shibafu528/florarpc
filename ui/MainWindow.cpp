#include "MainWindow.h"
#include "ImportsManageDialog.h"
#include "Editor.h"
#include "flora_constants.h"
#include "florarpc/workspace.pb.h"
#include <QStyle>
#include <QScreen>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <google/protobuf/util/json_util.h>

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent), protocolTreeModel(std::make_unique<ProtocolTreeModel>(this)),
          tabCloseShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this),
          treeMethodContextMenu(this) {
    ui.setupUi(this);

    connect(ui.actionOpen, &QAction::triggered, this, &MainWindow::onActionOpenTriggered);
    connect(ui.actionSaveWorkspace, &QAction::triggered, this, &MainWindow::onActionSaveWorkspaceTriggered);
    connect(ui.actionManageProto, &QAction::triggered, this, &MainWindow::onActionManageProtoTriggered);
    connect(ui.actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(ui.treeView, &QTreeView::clicked, this, &MainWindow::onTreeViewClicked);
    connect(ui.treeView, &QWidget::customContextMenuRequested, [=](const QPoint &pos) {
        const QModelIndex &index = ui.treeView->indexAt(pos);
        if (!index.parent().isValid() || !index.flags().testFlag(Qt::ItemFlag::ItemIsSelectable)) {
            // disabled node
            return;
        }

        treeMethodContextMenu.exec(ui.treeView->viewport()->mapToGlobal(pos));
    });
    connect(ui.editorTabs, &QTabWidget::tabCloseRequested, this, &MainWindow::onEditorTabCloseRequested);
    connect(&tabCloseShortcut, &QShortcut::activated, this, &MainWindow::onTabCloseShortcutActivated);

    ui.treeView->setModel(protocolTreeModel.get());

    treeMethodContextMenu.addAction("開く(&O)", [=]() {
        const QModelIndex &index = ui.treeView->indexAt(
                ui.treeView->viewport()->mapFromGlobal(treeMethodContextMenu.pos()));
        openMethod(index, false);
    });
    treeMethodContextMenu.addAction("新しいタブで開く(&N)", [=]() {
        const QModelIndex &index = ui.treeView->indexAt(
                ui.treeView->viewport()->mapFromGlobal(treeMethodContextMenu.pos()));
        openMethod(index, true);
    });

    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(),
                                    QGuiApplication::primaryScreen()->availableGeometry()));
}

void MainWindow::onActionOpenTriggered() {
    auto filenames = QFileDialog::getOpenFileNames(this, "Open proto", "",
                                                   "Proto definition files (*.proto)", nullptr);
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
            return;
        }
    }
    for (const auto &protocol : successes) {
        protocols.push_back(protocol);
        const auto index = protocolTreeModel->addProtocol(*protocol);
        ui.treeView->expandRecursively(index);
    }
}

void MainWindow::onActionSaveWorkspaceTriggered() {
    auto filename = QFileDialog::getSaveFileName(this, "Save workspace", "",
                                                 "FloraRPC Workspace (*.floraws)");
    if (filename.isEmpty()) {
        return;
    }

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

    for (int i = 0; i < ui.editorTabs->count(); i++) {
        auto editor = qobject_cast<Editor *>(ui.editorTabs->widget(i));
        if (editor != nullptr) {
            florarpc::Request *request = workspace.add_requests();
            editor->writeRequest(*request);
        }
    }

    std::string output;
    if (!workspace.SerializeToString(&output)) {
        QMessageBox::critical(this, "Save error", "ワークスペースの保存中にエラーが発生しました。\nワークスペース情報の書き出し準備に失敗しました。");
        return;
    }

    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox::critical(this, "Save error", "ワークスペースの保存中にエラーが発生しました。\n保存先のファイルを作成できません。");
        return;
    }
    file.write(QByteArray::fromStdString(output));
    file.close();

    ui.statusbar->showMessage("ワークスペースを保存しました!", 5000);
}

void MainWindow::onActionManageProtoTriggered() {
    auto dialog = std::make_unique<ImportsManageDialog>(this);
    dialog->setPaths(imports);
    dialog->exec();
    imports = dialog->getPaths();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (QMessageBox::information(this, "確認", "アプリケーションを終了します。よろしいですか？",
                                 QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
        event->ignore();
    }
}

void MainWindow::onTreeViewClicked(const QModelIndex &index) {
    openMethod(index, false);
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

void MainWindow::openMethod(const QModelIndex &index, bool forceNewTab) {
    if (!index.parent().isValid() || !index.flags().testFlag(Qt::ItemFlag::ItemIsSelectable)) {
        // disabled node
        return;
    }

    auto method = std::make_unique<Method>(ProtocolTreeModel::indexToMethod(index));
    auto methodName = method->getFullName();

    // Find exists tab
    if (!forceNewTab) {
        for (int i = 0; i < ui.editorTabs->count(); i++) {
            auto editor = qobject_cast<Editor *>(ui.editorTabs->widget(i));
            if (editor != nullptr && editor->getMethod().getFullName() == methodName) {
                ui.editorTabs->setCurrentIndex(ui.editorTabs->indexOf(editor));
                return;
            }
        }
    }

    auto editor = new Editor(std::move(method), syntaxDefinitions);
    const auto addedIndex = ui.editorTabs->addTab(editor, QString::fromStdString(methodName));
    ui.editorTabs->setCurrentIndex(addedIndex);
}
