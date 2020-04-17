#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QPixmap>
#include <QMainWindow>
#include <QSharedPointer>

#include "board.h"
#include "bot.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void updateStats(const Board<Bot> &board);
    QSharedPointer<Bot> updateStatBot(QSharedPointer<Bot> bot);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QSharedPointer<Bot> toStat{};

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
