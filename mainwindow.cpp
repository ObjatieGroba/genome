#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QClipboard>
#include <QKeyEvent>

#include "graphic.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(tr("Kek"));
    setCursor(Qt::CrossCursor);

    ui->widget_2->setData(&ui->widget->board->stat_data);
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::updateStats(const std::shared_ptr<Board<Bot>> &board) {
    if (toStat) {
        ui->textBrowser->setTextColor(QColor("White"));
        ui->textBrowser->setText(QString(toStat->info().c_str()));
    } else {
        ui->textBrowser->setText("");
    }
    ui->widget_2->update();
}


QSharedPointer<Bot> MainWindow::updateStatBot(QSharedPointer<Bot> bot) {
    auto tmp = toStat;
    toStat = std::move(bot);
    return tmp;
}


void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_C:
        if (toStat) {
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(toStat->info(true).c_str());
        }
        break;
    default:
        ui->widget->keyPressEvent(event);
    }
}
