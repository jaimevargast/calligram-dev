#ifndef STROKE_H
#define STROKE_H

#include <QPolygonF>
#include <QLineF>
#include <QPointF>
#include "planarcurve.h"

class Boundary;
class Stroke : public PlanarCurve
{
    struct strokePoint
    {
        QPointF p;
        QLineF lOrtho;
        QLineF rOrtho;
        QLineF lRay;        
        QLineF rRay;        
    };

    QPolygonF path;
    QVector<Stroke::strokePoint> points;
    Boundary * bnd;


    friend class Viewer;

public:
    Stroke();
    Stroke(QPolygonF);
    ~Stroke();    
    QPointF start() { return path.front(); }
    QPointF end() { return path.back(); }
    QLineF rayStart();
    QLineF rayFinish();

    QVector<QLineF> leftOrthos();
    QVector<QLineF> leftRayset();

    QVector<QLineF> rightOrthos();
    QVector<QLineF> rightRayset();

    void computeDistanceStrokeToBoundary(bool);

    qreal thickness();
    qreal TV1();
    qreal TV2();

private:
    void makeRays();
    void setLeftRayset(QVector<QLineF>);
    void setRightRayset(QVector<QLineF>);
    void smooth_t(QMap<int,qreal>&,int);
    QLineF pointToLineDist(const QPointF&, const QLineF&);
    QVector<QLineF> StrokeSegmentIntersection(PlanarCurve,const QVector<QLineF>&, bool);

};

#endif // STROKE_H
