#ifndef BOUNDARYSEGMENT_H
#define BOUNDARYSEGMENT_H

#include<QVector>
#include<QLineF>
#include<QMap>

class Viewer;

class boundarySegment
{
    friend class Viewer;

    QVector<QLineF> lines;
    qreal tot_length;
    QMap<int,qreal> cumulativeLength;

public:
    boundarySegment();
    boundarySegment(QVector<QLineF>);
    ~boundarySegment();
    qreal t_at_point(QPointF,int);
    QPointF point_at_t(qreal);

private:
    void computeLength(void);

};

#endif // BOUNDARYSEGMENT_H
