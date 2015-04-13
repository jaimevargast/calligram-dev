#include "boundarysegment.h"

boundarySegment::boundarySegment()
{

}

boundarySegment::boundarySegment(QVector<QLineF> s)
{
    lines = s;
    computeLength();
}

boundarySegment::~boundarySegment()
{

}

void boundarySegment::computeLength(void)
{
    tot_length=0;
    cumulativeLength.clear();
    for (int ix=0; ix<lines.size(); ix++)
    {
        tot_length += lines[ix].length();
        cumulativeLength.insert(ix,tot_length);
    }
}

qreal boundarySegment::t_at_point(QPointF p,int i)
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

QPointF boundarySegment::point_at_t(qreal t)
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
