#pragma once

#include <QGLWidget>
#include <QVector>
#include <Eigen/Dense>
#include "planarcurve.h"


class Stroke;
class Boundary;
class PlanarCurve;

namespace Ui {
class Viewer;
}

class MyQImage
        : public QImage{public: int x,y; double opacity; MyQImage(const QImage & img = QImage(),
                               int x = 0, int y = 0, double opacity = 1.0) : QImage(img), x(x), y(y), opacity(opacity) {}};

class Viewer : public QGLWidget
{
    Q_OBJECT

public:
    explicit Viewer(QWidget *parent = 0);
    ~Viewer();
    void paintEvent(QPaintEvent *);
    void keyPressEvent(QKeyEvent *);
    void mousePressEvent(QMouseEvent *);

    void clearAll();

    void extractBoundary();
    void mapStrokeToBoundary();
    bool saveFeatures(QString);
    bool saveStroke_and_Boundary();    


    QVector<MyQImage> images;
    QPolygonF points;
    Eigen::MatrixXd img_matrix;


    QVector<Boundary*> bnds;
    QVector<QPolygonF> bndPolys;

    QPolygonF sPoly;

    QVector<Stroke*> strokes;


    QString filename;

    QVector<QLineF> lrays;
    QVector<QLineF> rrays;    

private:
    Ui::Viewer *ui;

public slots:
};
