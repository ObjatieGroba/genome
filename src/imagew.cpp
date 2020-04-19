#include "imagew.h"
#include "ui_imagew.h"
#include "mainwindow.h"
#include <QPainter>
#include <QClipboard>
#include <QKeyEvent>

#include <cmath>

const double DefaultCenterX = 128;
const double DefaultCenterY = 128;
const double DefaultScale = 0.125f * 2;
const double ZoomInFactor = 0.9f;
const double ZoomOutFactor = 1 / ZoomInFactor;
const int ScrollStep = 20;

ImageW::ImageW(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImageW)
{
    ui->setupUi(this);

    board = std::make_shared<Board<Bot>>();
    board->run_threads(8);

    centerX = DefaultCenterX;
    centerY = DefaultCenterY;
    pixmapScale = DefaultScale;
    curScale = DefaultScale;

    thread.setBoard(board);

    connect(&thread, &RenderThread::renderedImage,
            this, &ImageW::updatePixmap);
}

ImageW::~ImageW()
{
    delete ui;
}


void ImageW::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (pixmap.isNull()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, tr("Rendering initial image, please wait..."));
        return;
    }

    if (curScale == pixmapScale) {
        painter.drawPixmap(pixmapOffset, pixmap);
    } else {
        double scaleFactor = pixmapScale / curScale;
        int newWidth = int(pixmap.width() * scaleFactor);
        int newHeight = int(pixmap.height() * scaleFactor);
        int newX = pixmapOffset.x() + (pixmap.width() - newWidth) / 2;
        int newY = pixmapOffset.y() + (pixmap.height() - newHeight) / 2;

        painter.save();
        painter.translate(newX, newY);
        painter.scale(scaleFactor, scaleFactor);

        QRectF exposed = painter.transform().inverted().mapRect(rect()).adjusted(-1, -1, 1, 1);
        painter.drawPixmap(exposed, pixmap, exposed);
        painter.restore();
    }

    if (auto pp = dynamic_cast<MainWindow *>(parentWidget()->parentWidget()); pp) {
        pp->updateStats(board);
    }
}

void ImageW::rerender() {
    thread.render(centerX, centerY, curScale, size());
}


void ImageW::resizeEvent(QResizeEvent * /* event */) {
    rerender();
}


void ImageW::keyPressEvent(QKeyEvent *event)
{
    if (auto_mode && event->key() != Qt::Key_A) {
        return;
    }
    switch (event->key()) {
        case Qt::Key_6:
            steps = 100000;
            board->make_steps(steps);
            rerender();
            break;
        case Qt::Key_5:
            steps = 10000;
            board->make_steps(steps);
            rerender();
            break;
        case Qt::Key_4:
            steps = 1000;
            board->make_steps(steps);
            rerender();
            break;
        case Qt::Key_3:
            steps = 100;
            board->make_steps(steps);
            rerender();
            break;
        case Qt::Key_2:
            steps = 10;
            board->make_steps(steps);
            rerender();
            break;
        case Qt::Key_1:
            steps = 1;
            board->make_steps(steps);
            rerender();
            break;
        case Qt::Key_R:
            ++board->rendertype;
            board->rendertype %= 5;
            rerender();
            break;
        case Qt::Key_W:
            board->wayrender = !board->wayrender;
            rerender();
            break;
        case Qt::Key_N:
            ++board->netrender;
            board->netrender %= 3;
            rerender();
            break;
        case Qt::Key_Q:
            board->resource_zone = !board->resource_zone;
            break;
        case Qt::Key_S:
            board->stat();
            break;
        case Qt::Key_A:
            auto_mode = !auto_mode;
            if (auto_mode) {
                rerender();
            }
            break;
        case Qt::Key_C:
            if (dynamic_cast<MainWindow*>(parent()->parent())->toStat) {
                QClipboard *clipboard = QApplication::clipboard();
                clipboard->setText(dynamic_cast<MainWindow*>(parent()->parent())->toStat->info(true).c_str());
            }
            break;
        case Qt::Key_F:
            if (dynamic_cast<MainWindow*>(parent()->parent())->toStat) {
                board->refill(dynamic_cast<MainWindow *>(parent()->parent())->toStat);
                rerender();
            }
            break;
        case Qt::Key_J:
            board->random();
            rerender();
            break;
        case Qt::Key_K:
            board->part_kill();
            rerender();
            break;
        case Qt::Key_Plus:
            zoom(ZoomInFactor);
            break;
        case Qt::Key_Minus:
            zoom(ZoomOutFactor);
            break;
        case Qt::Key_Left:
            scroll(-ScrollStep, 0);
            break;
        case Qt::Key_Right:
            scroll(+ScrollStep, 0);
            break;
        case Qt::Key_Down:
            scroll(0, -ScrollStep);
            break;
        case Qt::Key_Up:
            scroll(0, +ScrollStep);
            break;
        default:
            QWidget::keyPressEvent(event);
    }
}


void ImageW::wheelEvent(QWheelEvent *event) {
    int numDegrees = event->angleDelta().y() / 8;
    double numSteps = numDegrees / 15.0f;
    zoom(pow(ZoomInFactor, numSteps));
}


void ImageW::mousePressEvent(QMouseEvent *event) {
    if (event->pos().isNull()) {
        return;
    }
    if (event->button() == Qt::LeftButton)
        lastDragPos = event->pos();
    else if (event->button() == Qt::RightButton) {
        int halfWidth = size().width() / 2;
        int halfHeight = size().height() / 2;
        auto cell = board->get((event->pos().x() - halfWidth) * curScale + centerX,
                               (event->pos().y() - halfHeight) * curScale + centerY);
        QSharedPointer<Bot> bot;
        if (cell != None) {
            bot = board->getBotUnsave(cell);
            bot->isSelected = true;
        }
        if (auto pp = dynamic_cast<MainWindow *>(parentWidget()->parentWidget()); pp) {
            bot = pp->updateStatBot(std::move(bot));
            if (bot) {
                bot->isSelected = false;
            }
        } else {
            bot->isSelected = false;
        }
        rerender();
        update();
    }
}


void ImageW::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        pixmapOffset += event->pos() - lastDragPos;
        lastDragPos = event->pos();
        update();
    }
}


void ImageW::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        pixmapOffset += event->pos() - lastDragPos;
        lastDragPos = QPoint();

        int deltaX = (width() - pixmap.width()) / 2 - pixmapOffset.x();
        int deltaY = (height() - pixmap.height()) / 2 - pixmapOffset.y();
        scroll(deltaX, deltaY);
    }
}


void ImageW::updatePixmap(const QImage &image, double scaleFactor)
{
    if (!lastDragPos.isNull())
        return;

    pixmap = QPixmap::fromImage(image);
    pixmapOffset = QPoint();
    lastDragPos = QPoint();
    pixmapScale = scaleFactor;
    update();
    if (auto_mode) {
        board->make_steps(steps);
        rerender();
    }
}


void ImageW::zoom(double zoomFactor)
{
    curScale *= zoomFactor;
    update();
    rerender();
}


void ImageW::scroll(int deltaX, int deltaY)
{
    centerX += deltaX * curScale;
    centerY += deltaY * curScale;
    update();
    rerender();
}

