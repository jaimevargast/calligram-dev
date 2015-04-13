#include "linesegment.h"
lineSegment::lineSegment()
{

}

lineSegment::lineSegment(qreal x1, qreal y1, qreal x2, qreal y2)
{
    // Start point is to left of end point

    if (x1<=x2) // first point is to left of second point
    {
        ls.setP1(QPointF(x1,y1));
        ls.setP2(QPointF(x2,y2));
    }
    else        // first point is to right of second point
    {
        ls.setP1(QPointF(x2,y2));
        ls.setP2(QPointF(x1,y1));
    }
}

lineSegment::lineSegment(QLineF l)
{
    if (l.x1()<=l.x2())
    {
        ls.setP1(l.p1());
        ls.setP2(l.p2());
    }
    else
    {
        ls.setP1(l.p2());
        ls.setP2(l.p1());
    }
}

lineSegment::lineSegment(const lineSegment &obj)
{
    ls = obj.ls;
}

lineSegment::~lineSegment()
{

}

bool lineSegment::isLeft(QPointF p)
{
    return (p==ls.p1());
}
