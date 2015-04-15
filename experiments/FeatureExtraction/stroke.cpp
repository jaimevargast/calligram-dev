#include "stroke.h"
#include "planarcurve.h"
#include <QPolygonF>
#include <QPointF>
#include <QLineF>
#include <QVector>

Stroke::Stroke()
{
    path.clear();
    //lineNormals.clear();
    points.clear();
}

Stroke::Stroke(QPolygonF s) : PlanarCurve(s)
{        
    path = s;
    //lineNormals.clear();
    points.clear();
    makeRays();
}



Stroke::~Stroke()
{

}

void Stroke::makeRays()
{
    QVector<QPointF> lineNormals;
    qreal dx,dy;
    qreal lx,ly,rx,ry;
    QLineF line;
    Stroke::strokePoint tp;
    QPointF nn;

    for (int ix=0; ix<path.size(); ix++)
    {        
        if(ix!=path.size()-1)
        {
            //Get x and y components
            dx = path[ix+1].rx()- path[ix].rx();
            dy = path[ix+1].ry() - path[ix].ry();

            //Compute left and right normal components (unnormalized)
            lx = dy;
            ly = -dx;
            rx = -dy;
            ry = dx;

            //normalize
            nn = path.at(ix);
            nn += QPointF(lx,ly);
            line.setPoints(path.at(ix),nn);
            line.unitVector();

            lineNormals.append(QPointF(line.dx(),line.dy()));

            if (ix==0)                      //Special case: compute normal for first point
            {

                tp.p = path.at(ix);

                //Cast left ray
                tp.lRay = line;
                tp.lRay.setLength(10000);

                //Cast right ray

                nn = path.at(ix);
                nn += QPointF(rx,ry);
                line.setPoints(path.at(ix),nn);
                tp.rRay = line;
                tp.rRay.setLength(10000);

                //Append to points vector
                points.push_back(tp);

            }
            else                   //All other points: Point normal is average of neighbour line normals
            {
                tp.p = path.at(ix);

                //Compute (average) left and right normalized normal components (curr line norm + prev line norm /2)
                lx = ( lineNormals[ix].rx() + lineNormals[ix-1].rx() )/2;
                ly = ( lineNormals[ix].ry() + lineNormals[ix-1].ry() )/2;
                rx = -lx;
                ry = -ly;

                //Cast left ray
                nn = path.at(ix);
                nn += QPointF(lx,ly);
                line.setPoints(path.at(ix),nn);
                tp.lRay = line;
                tp.lRay.setLength(10000);

                //Cast right ray
                nn = path.at(ix);
                nn += QPointF(rx,ry);
                line.setPoints(path.at(ix),nn);
                tp.rRay = line;
                tp.rRay.setLength(10000);

                //Save structure
                points.push_back(tp);
            }
        }
        else
        {
            tp.p = path.at(ix);
            nn = lineNormals.at(ix-1);
            nn+= path.at(ix);

            //Cast left ray
            line.setPoints(path.at(ix),nn);
            tp.lRay = line;
            tp.lRay.setLength(10000);

            //Cast right ray
            nn = QPointF(-lineNormals[ix-1].rx(),-lineNormals[ix-1].ry());
            nn+= path[ix];
            line.setPoints(path[ix],nn);
            tp.rRay = line;
            tp.rRay.setLength(10000);

            //Save structure
            points.push_back(tp);
        }

        }
}


QLineF Stroke::rayStart()
{
    QLineF l(path[0],path[1]);
    QPointF p = path[0];
    p += QPointF(-l.dx(),-l.dy());
    l.setPoints(path[0],p);
    l.setLength(10000);
    return l;
}

QLineF Stroke::rayFinish()
{
    QLineF l(path[path.size()-2],path[path.size()-1]);
    QPointF p = path[path.size()-1];
    p += QPointF(l.dx(),l.dy());
    l.setPoints(path[path.size()-1],p);
    l.setLength(10000);
    return l;
}

QVector<QLineF> Stroke::leftRayset()
{
    QVector<QLineF> ans;
    for (int ix=0; ix<points.size(); ix++)
    {
        ans.append(points[ix].lRay);
    }

    return ans;
}

QVector<QLineF> Stroke::rightRayset()
{
    QVector<QLineF> ans;
    for (int ix=points.size()-1; ix>=0; ix--)
    {
        ans.append(points[ix].rRay);
    }

    return ans;
}

QVector<qreal> Stroke::thickness()
{
    QVector<qreal> ans;
    qreal d;
    for (int i=0; i<points.size(); i++)
    {
        d = points[i].lRay.length();
        d+= points[i].rRay.length();
        ans.push_back(d);
    }

    return ans;
}
