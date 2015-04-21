#include "planarcurve.h"
#include<QTextStream>
#include<QtMath>

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062

PlanarCurve::PlanarCurve()
{

}

PlanarCurve::PlanarCurve(QPolygonF poly)
{
    lines.clear();
    for (int i=0; i<poly.size()-1; i++)
    {
        QLineF l(poly[i],poly[i+1]);
        lines.push_back(l);
    }
    computeLength();
}

PlanarCurve::PlanarCurve(QVector<QLineF> s)
{
    lines = s;
    computeLength();
}

PlanarCurve::~PlanarCurve()
{

}

void PlanarCurve::computeLength(void)
{
    tot_length=0;
    cumulativeLength.clear();
    for (int ix=0; ix<lines.size(); ix++)
    {
        tot_length += lines[ix].length();
        cumulativeLength.insert(ix,tot_length);
    }
}

qreal PlanarCurve::t_at_point(QPointF p,int i)
{
    qreal l = 0;


    if(i>1)
    {
        l = cumulativeLength.value(i-1);    //get cumulative length at i-1
    }

     QLineF dummy(lines[i].p1(),p);
     l += dummy.length();

     l = l/tot_length;
     return l;
}

QPointF PlanarCurve::point_at_t(qreal t)
{
    QPointF p;
    if ((t>=0.0)&&(t<=1.0))
    {
        qreal d = tot_length*t;
        int ix=0;

        while(cumulativeLength.value(ix)<=d)
        {
            ix++;
        }

        QLineF dummy = lines[ix];
        if (ix==0)
            dummy.setLength(d);
        else
            dummy.setLength(d-cumulativeLength.value(ix-1));

        p = dummy.p2();
    }

    return p;
}

QVector<qreal> PlanarCurve::k_signature()
{
    QVector<qreal> answer;
    answer.resize(lines.size()+1);
    qreal a;
    if (lines.front().p1()==lines.last().p2()) //curve is closed
    {
        answer[0] = curvature(lines.last(),lines.front());
        answer.last() = answer[0];
    }
    else
    {
        answer.front() = 0;
        answer.back() = 0;
    }
    for (int i=1; i<lines.size(); i++)
    {
        a = curvature(lines[i-1],lines[i]);
        answer[i] = a;
    }
//    for (auto i : answer)
//        QTextStream(stdout) << "k=" << i << "\n";
    return answer;
}

qreal PlanarCurve::curvature(QLineF l1, QLineF l2)
{
    qreal a = turningAngle(l1,l2);
    qreal k = 2*qSin(a/2);
    return k;
}

qreal PlanarCurve::turningAngle(QLineF l1, QLineF l2)
{
    qreal a= l1.angleTo(l2);
    if (a>180)
        a = -(360-a);
    a = a*PI/180;
    return a;
}
