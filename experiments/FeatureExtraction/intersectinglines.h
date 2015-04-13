#ifndef INTERSECTINGLINES_H
#define INTERSECTINGLINES_H

class BSTree;
class lineSegment;
class intersectingLines
{
    class Points
    {
        QPointF point;
        int type;           // -1:left 1:right 0:intersection
        lineSegment * l;    // Pointer to line point belongs to, or intersecting line
        lineSegment * l2;   // Pointer to intersecting line

    public:
        Points(QPointF, lineSegment*);
        Points(QPointF, lineSegment*, lineSegment*);
        Points(qreal, qreal, lineSegment*);
        Points(qreal, qreal, lineSegment*, lineSegment*);
        QPointF getPoint() { return point; }
        int getType() { return type; }
        lineSegment* getLine() { return l; }
        lineSegment* getLine2() { return l2; }
    };

public:
    intersectingLines();
    ~intersectingLines();
};

#endif // INTERSECTINGLINES_H
