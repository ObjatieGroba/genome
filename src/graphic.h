#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <QWidget>

namespace Ui {
class Graphic;
}

class Graphic : public QWidget
{
    Q_OBJECT

public:
    struct Data {
        std::string name;
        std::vector<unsigned> data;
        unsigned max{};

        Data() = default;
        Data(std::string name_) : name(std::move(name_)) { }

        void push(unsigned value) {
            max = std::max(value, max);
            data.push_back(value);
        }
    };

    explicit Graphic(QWidget *parent = nullptr);
    ~Graphic();

    void setData(std::vector<Data> *data_);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    // void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void on_pushButton_prev_clicked();

    void on_pushButton_next_clicked();

private:
    Ui::Graphic *ui;
    std::vector<Data> *data;
    unsigned id;
    unsigned start;
    int mouse_x;
    unsigned last_max;
    QPoint last_point;
    double perwidth;
    bool isFull = false;
};

#endif // GRAPHIC_H
