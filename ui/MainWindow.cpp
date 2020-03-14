#include "MainWindow.h"
#include "../entity/Protocol.h"
#include "ImportsManageDialog.h"
#include "Editor.h"
#include <QStyle>
#include <QScreen>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QClipboard>
#include <QJsonDocument>
#include <QJsonObject>
#include <Theme>
#include <memory>
#include <google/protobuf/dynamic_message.h>

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent), protocolTreeModel(std::make_unique<ProtocolTreeModel>(this)) {
    ui.setupUi(this);

    connect(ui.actionOpen, &QAction::triggered, this, &MainWindow::onActionOpenTriggered);
    connect(ui.actionManageProto, &QAction::triggered, this, &MainWindow::onActionManageProtoTriggered);
    connect(ui.actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(ui.treeView, &QTreeView::clicked, this, &MainWindow::onTreeViewClicked);
    connect(ui.editorTabs, &QTabWidget::tabCloseRequested, [=](int index) {
        auto editor = ui.editorTabs->widget(index);
        ui.editorTabs->removeTab(index);
        delete editor;
    });

    ui.treeView->setModel(protocolTreeModel.get());

    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(),
            QGuiApplication::primaryScreen()->availableGeometry()));
}

void MainWindow::onActionOpenTriggered() {
    auto filename = QFileDialog::getOpenFileName(this, "Open proto", "",
            "Proto definition files (*.proto)", nullptr);
    if (filename.isEmpty()) {
        return;
    }
    QFileInfo file(filename);
    for (auto &p : protocols) {
        if (p->getSource() == file) {
            QMessageBox::warning(this, "Load error", "このファイルはすでに読み込まれています。");
            return;
        }
    }
    try {
        const auto protocol = std::make_shared<Protocol>(file, imports);
        protocols.push_back(protocol);
        const auto index = protocolTreeModel->addProtocol(*protocol);
        ui.treeView->expandRecursively(index);
    } catch (ProtocolLoadException &e) {
        QString message = "Protoファイルの読込中にエラーが発生しました。\n";
        QTextStream stream(&message);

        for (const auto& err : *e.errors) {
            if (&err != &e.errors->front()) {
                stream << '\n';
            }
            stream << QString::fromStdString(err);
        }
        QMessageBox::critical(this, "Load error", message);
    }
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
    if (!index.parent().isValid() || !index.flags().testFlag(Qt::ItemFlag::ItemIsSelectable)) {
        // disabled node
        return;
    }

    auto descriptor = ProtocolTreeModel::indexToMethodDescriptor(index);
    auto editor = new Editor(descriptor, syntaxDefinitions);
    const auto addedIndex = ui.editorTabs->addTab(editor, QString::fromStdString(descriptor->full_name()));
    ui.editorTabs->setCurrentIndex(addedIndex);
}
