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

    void updateStats(const std::shared_ptr<Board<Bot>> &board);
    QSharedPointer<Bot> updateStatBot(QSharedPointer<Bot> bot);

    QSharedPointer<Bot> toStat{};

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
