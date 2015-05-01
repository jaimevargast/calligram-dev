#ifndef BOUNDARY_H
#define BOUNDARY_H

#include <QPolygon>
#include <QLineF>
#include <QPair>


class Boundary
{
    QPolygonF boundaryPoly;
    QVector<QLineF> boundaryLines;
    //bool isHole;

public:
    Boundary();
    Boundary(QPolygonF);
    ~Boundary();

    void setBoundary(QPolygonF);                                // Clears current boundary object and populates it with polygon
    QPolygonF getPolygon() { return boundaryPoly; }
    QVector<QLineF> getLines() { return boundaryLines; }
    QVector<QLineF> getSegment(int,int);
    QPair<int,QPointF> closestIntersection(const QLineF&);      // Given a ray, returns index into boundaryLines as well as coordinates of nearest intersection, if it exists; -1 otherwise
    qreal area();

private:
    void makeLines();
    bool isClockwise();
    void makeClockwise();
};

#endif // BOUNDARY_H
