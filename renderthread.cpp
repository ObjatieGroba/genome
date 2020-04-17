#include "renderthread.h"

#include <QImage>
#include <cmath>

RenderThread::RenderThread(QObject *parent)
    : QThread(parent)
{
    restart = false;
    abort = false;
}

RenderThread::~RenderThread()
{
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
}

void RenderThread::render(double centerX, double centerY, double scaleFactor,
                          QSize resultSize)
{
    QMutexLocker locker(&mutex);

    this->centerX = centerX;
    this->centerY = centerY;
    this->scaleFactor = scaleFactor;
    this->resultSize = resultSize;

    if (!isRunning()) {
        start(LowPriority);
    } else {
        restart = true;
        condition.wakeOne();
    }
}

void RenderThread::run()
{
    forever {
        mutex.lock();
        QSize resultSize = this->resultSize;
        double scaleFactor = this->scaleFactor;
        double centerX = this->centerX;
        double centerY = this->centerY;
        mutex.unlock();

        int halfWidth = resultSize.width() / 2;
        int halfHeight = resultSize.height() / 2;

        int startY = 0, startX = 0;
        int finishY = resultSize.height(), finishX = resultSize.width();

        image = QImage(resultSize, QImage::Format_RGB32);
        image.fill(0);

        uint32_t *scanLine;
        for (int y = startY; y < finishY; ++y) {
            double ady = centerY + ((y - halfHeight) * scaleFactor);
            int ay = int(ady);
            scanLine = reinterpret_cast<uint32_t *>(image.scanLine(y)) + startX;
            if (ady >= 0 && ay < static_cast<int>(board->getHeight())) {
                for (int x = startX; x < finishX; ++x) {
                    double adx = centerX + ((x - halfWidth) * scaleFactor);
                    int ax = int(adx);
                    if (adx >= 0 && ax < static_cast<int>(board->getWidth())) {
                        auto cell = board->get(ax, ay);
                        if (board->netrender == 1 && ((ady - static_cast<double>(ay)) < scaleFactor ||
                                                      (adx - static_cast<double>(ax)) < scaleFactor)) {
                            *scanLine = qRgb(0, 0, 0);
                            ++scanLine;
                            continue;
                        }
                        if (cell == None) {
                            if (board->netrender == 2 && ((ady - static_cast<double>(ay)) < scaleFactor ||
                                                          (adx - static_cast<double>(ax)) < scaleFactor)) {
                                *scanLine = qRgb(0, 0, 0);
                                ++scanLine;
                                continue;
                            }
                            *scanLine = qRgb(255, 255, 255);
                            ++scanLine;
                            continue;
                        }
                        auto bot = board->getBotUnsave(cell);
                        if (!bot) {
                            *scanLine = qRgb(255, 255, 255);
                            ++scanLine;
                            continue;
                        }
                        if (board->netrender == 2 && ay > 0 && ((ady - static_cast<double>(ay)) < scaleFactor)) {
                            auto neib = board->get(ax, ay - 1);
                            if (neib == None) {
                                *scanLine = qRgb(0, 0, 0);
                                ++scanLine;
                                continue;
                            }
                            auto nbot = board->getBotUnsave(neib);
                            if (!nbot) {
                                *scanLine = qRgb(255, 255, 255);
                                ++scanLine;
                                continue;
                            }
                            if (!bot->similar(*nbot)) {
                                *scanLine = qRgb(0, 0, 0);
                                ++scanLine;
                                continue;
                            }
                        }
                        if (board->netrender == 2 && ax > 0 && ((adx - static_cast<double>(ax)) < scaleFactor)) {
                            auto neib = board->get(ax - 1, ay);
                            if (neib == None) {
                                *scanLine = qRgb(0, 0, 0);
                                ++scanLine;
                                continue;
                            }
                            auto nbot = board->getBotUnsave(neib);
                            if (!nbot) {
                                *scanLine = qRgb(255, 255, 255);
                                ++scanLine;
                                continue;
                            }
                            if (!bot->similar(*nbot)) {
                                *scanLine = qRgb(0, 0, 0);
                                ++scanLine;
                                continue;
                            }
                        }
                        if (board->wayrender) {
                            double deltax = adx - static_cast<double>(ax);
                            double deltay = ady - static_cast<double>(ay);
                            switch (bot->way) {
                            case Left:
                                if (deltax < 0.125f && deltay < 0.5f + 0.125f && deltay > 0.5f - 0.125f) {
                                    *scanLine = qRgb(0, 0, 0);
                                    ++scanLine;
                                    continue;
                                }
                                break;
                            case Top:
                                if (deltay < 0.125f && deltax < 0.5f + 0.125f && deltax > 0.5f - 0.125f) {
                                    *scanLine = qRgb(0, 0, 0);
                                    ++scanLine;
                                    continue;
                                }
                                break;
                            case Right:
                                if (deltax > 1.0f - 0.125f && deltay < 0.5f + 0.125f && deltay > 0.5f - 0.125f) {
                                    *scanLine = qRgb(0, 0, 0);
                                    ++scanLine;
                                    continue;
                                }
                                break;
                            case Bottom:
                                if (deltay > 1.0f - 0.125f && deltax < 0.5f + 0.125f && deltax > 0.5f - 0.125f) {
                                    *scanLine = qRgb(0, 0, 0);
                                    ++scanLine;
                                    continue;
                                }
                                break;
                            }
                        }
                        if (bot->isSelected) {
                            double deltax = adx - static_cast<double>(ax);
                            double deltay = ady - static_cast<double>(ay);
                            if (abs(1.0f - deltax - deltay) < 0.05f || abs(deltax - deltay) < 0.05f) {
                                if (deltax < 0.5f + 0.2f && deltax > 0.5f - 0.2f) {
                                    *scanLine = qRgb(0, 0, 0);
                                    ++scanLine;
                                    continue;
                                }
                            }
                        }
                        switch (board->rendertype) {
                        case 1:
                        {
                            int r = bot->energy + 20;
                            if (r < 20) {
                                r = 20;
                            } else if (r > 255) {
                                r = 255;
                            }
                            *scanLine = qRgb(r, 0, 0);
                            break;
                        }
                        case 0:
                        {
                            int r = bot->energy + 80;
                            if (r < 80) {
                                r = 80;
                            } else if (r > 255) {
                                r = 255;
                            }
                            unsigned cc = 80;
                            if (!bot->isAlive()) {
                                cc -= 80;
                                r -= 80;
                            }
                            if (bot->type == 0) {
                                *scanLine = qRgb(r, cc, cc);
                            } else if (bot->type == 1) {
                                *scanLine = qRgb(cc, r, cc);
                            } else if (bot->type == 2) {
                                *scanLine = qRgb(cc, cc, r);
                            } else if (bot->type == 3) {
                                *scanLine = qRgb(r, r, cc);
                            } else {
                                *scanLine = qRgb(r, r / 2, cc);
                            }
                            break;
                        }
                        case 2:
                        {
                            unsigned msg = bot->msgs[0] + bot->msgs[1] + bot->msgs[2] + bot->msgs[3];
                            ++msg;
                            msg *= 11;
                            std::array<unsigned char, 7> colors = {255, 212, 169, 126, 83, 40, 0};
                            *scanLine = qRgb(colors[msg % 7], colors[(msg / 7) % 7], colors[(msg / 49) % 7]);
                            break;
                        }
                        case 3:
                        {
                            int r = bot->minerals + 20;
                            if (r < 20) {
                                r = 20;
                            } else if (r > 255) {
                                r = 255;
                            }
                            *scanLine = qRgb(0, 0, r);
                            break;
                        }
                        }
                    }
                    ++scanLine;
                }
            }
        }


        if (abort)
            return;
        if (!restart) {
            emit renderedImage(image, scaleFactor);
        }

        mutex.lock();
        if (!restart)
            condition.wait(&mutex);
        restart = false;
        mutex.unlock();
    }
}

