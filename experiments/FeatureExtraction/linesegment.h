#ifndef LINESEGMENT_H
#define LINESEGMENT_H

#include <QLineF>

class lineSegment
{
    QLineF ls;

public:    
    lineSegment();
    lineSegment(qreal, qreal, qreal, qreal);
    lineSegment(QLineF);
    lineSegment(const lineSegment &obj);
    ~lineSegment();
    QPointF start() { return ls.p1(); }
    QPointF end()   { return ls.p2(); }
    bool isLeft(QPointF);
};

#endif // LINESEGMENT_H
