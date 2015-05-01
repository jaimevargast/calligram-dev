#ifndef PLANARCURVE_H
#define PLANARCURVE_H

#include<QVector>
#include<QLineF>
#include<QMap>
#include<QPolygonF>

class Viewer;
class Stroke;

class PlanarCurve
{
    friend class Stroke;
    friend class Viewer;

    QVector<QLineF> lines;
    qreal tot_length;
    QMap<int,qreal> cumulativeLength;

public:
    PlanarCurve();
    PlanarCurve(QPolygonF);
    PlanarCurve(QVector<QLineF>);
    ~PlanarCurve();
    qreal t_at_point(QPointF,int);
    QPointF point_at_t(qreal);
    QVector<qreal> k_signature();

private:
    void computeLength(void);
    qreal turningAngle(QLineF,QLineF);
    qreal curvature(QLineF, QLineF);

};

#endif // PLANARCURVE_H
