#include "boundary.h"
#include <QtAlgorithms>
#include <QPolygonF>
#include <QLineF>
#include <QPair>
#include <QTextStream>

Boundary::Boundary()
{
    boundaryPoly.clear();
    boundaryLines.clear();
}

Boundary::Boundary(QPolygonF b)
{
    boundaryPoly = b;
    boundaryLines.clear();
    makeClockwise();
    makeLines();
}

Boundary::~Boundary()
{

}

void Boundary::setBoundary(QPolygonF b)
{
    boundaryPoly.clear();
    boundaryLines.clear();
    boundaryPoly=b;
    makeClockwise();
    makeLines();
}

QVector<QLineF> Boundary::getSegment(int start, int end)        // Assumes start!=end
{
    QVector<QLineF> segment;

    if (start!=end)
    {
        int ix=start;
        segment.append(boundaryLines[ix]);
        ix++;

        while (ix!=end)
        {
            if(ix==boundaryLines.size())
                ix=0;
            segment.append(boundaryLines[ix]);
            ix++;
        }
    }

    return segment;
}

 QPair<int,QPointF> Boundary::closestIntersection(const QLineF& l)
 {
     QVector<QPair<int,QPointF>> temp;
     QPointF * p = new QPointF();

     if (!boundaryLines.empty())
     {
         for (int ix=0; ix<boundaryLines.size(); ix++)
         {
             if(boundaryLines[ix].intersect(l,p)==QLineF::BoundedIntersection)
             {
                 temp.append(qMakePair(ix,*p));
             }
         }

         if (!temp.empty())
         {
             //get closest
             qreal d;
             qreal mind = 100000000;
             QLineF dummy;
             int minix=0;

             for (int ix=0; ix<temp.size(); ix++)
             {
                 dummy.setPoints(l.p1(),temp[ix].second);
                 d = dummy.length();
                 if (d<mind)
                 {
                     mind = d;
                     minix = ix;
                 }
             }

             return temp[minix];
         }
     }

     return qMakePair(-1,QPointF());
 }

//************************* Private functions **************************************
void Boundary::makeLines()
{
    for (int ix=0; ix<boundaryPoly.size()-1; ix++)
    {
        QLineF l(boundaryPoly[ix],boundaryPoly[ix+1]);
        boundaryLines.append(l);
    }
}

bool Boundary::isClockwise()
{
    qreal tot = 0;
    for (int ix=0; ix<boundaryPoly.size()-1; ix++)
    {
         // Shoelace formula ... sum of (x2-x1)(y2+y1) for all edges in polygon
         tot += (boundaryPoly[ix+1].rx()-boundaryPoly[ix].rx())*(boundaryPoly[ix+1].ry()+boundaryPoly[ix].ry());
    }

    if (tot<0)  // if negative, clockwise (because y-axis inverted)
        return true;
    else
        return false;
}

void Boundary::makeClockwise()
{
    if (!isClockwise())
    {
        QPolygonF temp;
        qCopyBackward(boundaryPoly.begin(), boundaryPoly.end(), temp.end());
        boundaryPoly=temp;
    }
}

qreal Boundary::area()
{
    qreal a=0;
    qreal aux1=0;
    qreal aux2=0;
    for (int i=0; i<boundaryPoly.size()-1; i++)
    {
        aux1 += (boundaryPoly[i].rx()   *  boundaryPoly[i+1].ry());
        aux2 += (boundaryPoly[i+1].rx() *  boundaryPoly[i].ry());
    }
    a = qAbs(aux1 + (boundaryPoly.last().rx() * boundaryPoly.front().ry()) - aux2 - (boundaryPoly.front().rx() * boundaryPoly.last().ry()));
    a = a/2;

    return a;
}
