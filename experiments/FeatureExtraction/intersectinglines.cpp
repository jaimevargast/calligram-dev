#include "intersectinglines.h"
#include "linesegment.h"
#include <QPointF>

intersectingLines::intersectingLines()
{

}

intersectingLines::~intersectingLines()
{

}

//************* Point class
intersectingLines::Points::Points(qreal x, qreal y, lineSegment *line)
{
    point.setX(x);
    point.setY(y);
    l = line;
    QPointF temp(x,y);
    if (line->isLeft(temp))
}

intersectingLines::Points::Points(QPointF p, lineSegment *line)
{
    point = p;
    l = line;
}

intersectingLines::Points::Points(QPointF p, lineSegment *line, lineSegment *line2)
{

}
