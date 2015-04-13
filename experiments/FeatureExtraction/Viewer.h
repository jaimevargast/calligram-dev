#pragma once

#include <QGLWidget>
#include <QVector>
#include <Eigen/Dense>

class Stroke;
class Boundary;

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
    void computeDistanceStrokeToBoundary(Stroke *, bool);
    QVector<QLineF> parametrizedBoundaryIntersection(const QVector<QLineF>&, QVector<QLineF>&, bool);
    QLineF pointToLineDist(const QPointF &, const QLineF &);
    void clearAll();


    //QVector<QPolygonF> polys;
    QVector<MyQImage> images;
    QPolygonF points;    
    Eigen::MatrixXd img_matrix;

    Boundary * bnd;
    QPolygonF bndPoly;
    //QVector<QLineF> lBoundary;
    QVector<Stroke*> strokes;    
    QVector<QLineF> rays;


private:
    Ui::Viewer *ui;

public slots:
};