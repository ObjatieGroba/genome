#include "graphic.h"
#include "ui_graphic.h"

#include <QPainter>
#include <QKeyEvent>

Graphic::Graphic(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Graphic)
{
    ui->setupUi(this);
    setMouseTracking(true);
}

Graphic::~Graphic()
{
    delete ui;
}

void Graphic::setData(std::vector<Data> *data_) {
    data = data_;
    if (data && !data->empty()) {
        id = 0;
        ui->label_name->setText((*data)[0].name.c_str());
    }
}

void Graphic::paintEvent(QPaintEvent * /* event */)
{
    if (!data || data->empty()) {
        return;
    }
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
    const auto & d = (*data)[id].data;
    if (d.empty()) {
        return;
    }
    painter.setPen(QPen(Qt::blue, 3));
    auto h = painter.device()->height() - 45;
    auto w = painter.device()->width();

    start = 0;
    if (!isFull && d.size() > w / 2) {
        start = d.size() - w / 2;
    }
    perwidth = d.size() - start;
    perwidth /= w;
    last_max = 0;
    if (start == 0) {
        last_max = (*data)[id].max;
    } else {
        for (auto i = start; i < d.size() - 1; ++i) {
            if (d[i] > last_max) {
                last_max = d[i];
            }
        }
    }
    if (last_max == 0) {
        return;
    }

    auto old_hh = (h * d[start]) / last_max;
    auto old_i = start;
    for (double ii = start + perwidth; ; ii += perwidth) {
        auto i = static_cast<unsigned>(ii + perwidth);
        if (i >= d.size()) {
            break;
        }
        if (i == old_i) {
            continue;
        }
        auto hh = (h * d[i]) / last_max;
        painter.drawLine((old_i - start) / perwidth, h - old_hh, (i - start) / perwidth, h - hh);
        old_hh = hh;
        old_i = i;
    }
    if (mouse_x > 0) {
        unsigned i = start + mouse_x * perwidth;
        if (i > d.size()) {
            return;
        }
        ui->label_extra->setText(std::to_string(d[i]).c_str());
        auto hh = (h * d[i]) / last_max;
        last_point = QPoint(mouse_x, h - hh);
        painter.setPen(QPen(Qt::red, 8));
        painter.drawPoint(last_point);
    }
}


void Graphic::mousePressEvent(QMouseEvent *event) {
    if (!data || data->empty()) {
        return;
    }
    if (event->button() == Qt::RightButton) {
        isFull = !isFull;
    }
    mouse_x = event->pos().x();
    update();
}

//void Graphic::mouseMoveEvent(QMouseEvent *event) {
//    if (!data || data->empty()) {
//        return;
//    }
//    mouse_x = event->pos().x();

//    QPainter painter(this);

//    unsigned i = start + mouse_x * perwidth;
//    if (i > (*data)[id].data.size()) {
//        return;
//    }
//    auto hh = (painter.device()->height() * (*data)[id].data[i]) / last_max;
//    painter.setPen(QPen(Qt::white, 8));
//    painter.drawPoint(last_point);
//    painter.setPen(QPen(Qt::blue, 3));
//    painter.drawPoint(last_point);
//    last_point = QPoint(mouse_x, painter.device()->height() - hh);

//    painter.setPen(QPen(Qt::red, 8));
//    painter.drawPoint(last_point);
//}

void Graphic::on_pushButton_prev_clicked() {
    if (id == 0) {
        id = data->size();
    }
    --id;
    ui->label_name->setText((*data)[id].name.c_str());
    update();
}

void Graphic::on_pushButton_next_clicked() {
    ++id;
    id %= data->size();
    ui->label_name->setText((*data)[id].name.c_str());
    update();
}
