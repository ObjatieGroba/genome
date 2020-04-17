#ifndef IMAGEW_H
#define IMAGEW_H

#include <QWidget>
#include "renderthread.h"

#include "board.h"
#include "bot.h"

namespace Ui {
class ImageW;
}

class ImageW : public QWidget
{
    Q_OBJECT

public:
    friend class MainWindow;

    explicit ImageW(QWidget *parent = nullptr);
    ~ImageW();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void updatePixmap(const QImage &image, double scaleFactor);
    void zoom(double zoomFactor);

private:
    void rerender();

    void scroll(int deltaX, int deltaY);
    QPixmap pixmap;
    QPoint pixmapOffset;
    QPoint lastDragPos;
    double centerX;
    double centerY;
    double pixmapScale;
    double curScale;

    std::shared_ptr<Board<Bot>> board;
    RenderThread thread;
    unsigned steps = 1;
    bool auto_mode = false;
    bool is_rendering = false;

    Ui::ImageW *ui;
};

#endif // IMAGEW_H
