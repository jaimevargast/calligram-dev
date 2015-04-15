#ifndef STROKE_H
#define STROKE_H

#include <QPolygonF>
#include <QLineF>
#include <QPointF>
#include "planarcurve.h"

class Stroke : public PlanarCurve
{
    struct strokePoint
    {
        QPointF p;        
        QLineF lRay;        
        QLineF rRay;        
    };

    QPolygonF path;
    QVector<Stroke::strokePoint> points;


    friend class Viewer;

public:
    Stroke();
    Stroke(QPolygonF);
    ~Stroke();    
    QPointF start() { return path.front(); }
    QPointF end() { return path.back(); }
    QLineF rayStart();
    QLineF rayFinish();
    QVector<QLineF> leftRayset();
    QVector<QLineF> rightRayset();
    QVector<qreal> thickness();

private:
    void makeRays();

};

#endif // STROKE_H
