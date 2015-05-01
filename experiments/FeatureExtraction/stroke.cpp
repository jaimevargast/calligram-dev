#include "stroke.h"
#include "planarcurve.h"
#include "boundary.h"
#include <QPolygonF>
#include <QPointF>
#include <QLineF>
#include <QVector>
#include <QTextStream>
#include <QtMath>

Stroke::Stroke()
{
    path.clear();    
    points.clear();
    bnd = NULL;
}

Stroke::Stroke(QPolygonF s) : PlanarCurve(s)
{        
    path = s;    
    points.clear();
    makeRays();
    bnd = NULL;
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
                tp.lOrtho = line;
                tp.lOrtho.setLength(10000);

                //Cast right ray

                nn = path.at(ix);
                nn += QPointF(rx,ry);
                line.setPoints(path.at(ix),nn);
                tp.rOrtho = line;
                tp.rOrtho.setLength(10000);

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
                tp.lOrtho = line;
                tp.lOrtho.setLength(10000);

                //Cast right ray
                nn = path.at(ix);
                nn += QPointF(rx,ry);
                line.setPoints(path.at(ix),nn);
                tp.rOrtho = line;
                tp.rOrtho.setLength(10000);

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
            tp.lOrtho = line;
            tp.lOrtho.setLength(10000);

            //Cast right ray
            nn = QPointF(-lineNormals[ix-1].rx(),-lineNormals[ix-1].ry());
            nn+= path[ix];
            line.setPoints(path[ix],nn);
            tp.rOrtho = line;
            tp.rOrtho.setLength(10000);

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

QVector<QLineF> Stroke::leftOrthos()
{
    QVector<QLineF> ans;
    for (int ix=0; ix<points.size(); ix++)
    {
        ans.append(points[ix].lOrtho);
    }

    return ans;
}

QVector<QLineF> Stroke::rightOrthos()
{
    QVector<QLineF> ans;
    for (int ix=points.size()-1; ix>=0; ix--)
    {
        ans.append(points[ix].rOrtho);
    }

    return ans;
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

void Stroke::setLeftRayset(QVector<QLineF> rayset)
{
    if (rayset.size()==points.size()) {
        for (int ix=0; ix<points.size(); ix++) {
            points[ix].lRay = rayset[ix];
        }
    }
}

void Stroke::setRightRayset(QVector<QLineF> rayset)
{
    if (rayset.size()==points.size()) {
        for (int ix=points.size()-1; ix>=0; ix--) {
            points[ix].rRay = rayset[ix];
        }
    }
}

qreal Stroke::thickness() //Thickness - boundary area/stroke length
{
    qreal a=bnd->area();

    QTextStream(stdout) << "Area: " << a << "\n";
    QTextStream(stdout) << "Stroke Length: " << this->tot_length << "\n";
    a = a/this->tot_length;
    QTextStream(stdout) << "Stroke Thickness (area/length): " << a << "\n";

    return a;
}

void Stroke::computeDistanceStrokeToBoundary(bool fix) //Wrapper, computes stroke-to-boundary distance
{
    QLineF r;
    int i1,i2;


    if (!bnd==NULL) {

        // Compute splice points
        //r = s->rayStart();
        r = this->rayStart();

        //i1 = b->closestIntersection(r).first;
        i1 = bnd->closestIntersection(r).first;

        //r = s->rayFinish();
        r = this->rayFinish();
        i2 = bnd->closestIntersection(r).first;

        // Split boundary
        PlanarCurve segment1 = bnd->getSegment(i1,i2);
        PlanarCurve segment2 = bnd->getSegment(i2,i1);

        setLeftRayset(StrokeSegmentIntersection(segment1,this->leftOrthos(),fix));
        setRightRayset(StrokeSegmentIntersection(segment2,this->rightOrthos(),fix));
    }
}

QVector<QLineF> Stroke::StrokeSegmentIntersection(PlanarCurve seg, const QVector<QLineF> &rayset, bool fix) // Computes stroke-to-boundary distance for either left or right side separately
{
    QMap<int,int> riMap;
    QMap<int,qreal> rtMap;
    QVector<int> aux;
    QVector<QPair<int,QPointF>> temp;
    QVector<QLineF> answer;

    QPointF * p = new QPointF();
    QLineF dummy;
    qreal d,mind,t;
    int k;

    for (int i=0; i<rayset.size(); i++)
    {
        for (int j=0; j<seg.lines.size(); j++)
        {
            if(rayset[i].intersect(seg.lines[j],p)==QLineF::BoundedIntersection)
            {
                temp.append(qMakePair(j,*p));

            }
        }

        if (!temp.empty())
        {
            //get closest
            mind = 100000000;
            k=0;

            for (int ix=0; ix<temp.size(); ix++)
            {
                dummy.setPoints(rayset[i].p1(),temp[ix].second);
                d = dummy.length();
                if (d<mind)
                {
                    mind = d;
                    k = ix;
                }
            }
            riMap.insert(i,temp[k].first);                          //Ray i intersects with boundary segment j
            t = seg.t_at_point(temp[k].second,temp[k].first);
            rtMap.insert(i,t);
            aux.push_back(temp[k].first);
            answer.append(QLineF(rayset[i].p1(),temp[k].second));
        }
        else
        {
            riMap.insert(i,-1);             // Ray i does not intersect with any boundary segment
            //aux.push_back(-1);
            answer.append(QLineF(rayset[i].p1(),rayset[i].p1())); //just a point
        }
        temp.clear();
    }

    if(fix)
    {

        //Processing:
        // 1. Detect and mark intersections
        for (int ix=0; ix<answer.size(); ix++)
        {
            for (int jx=ix; jx<answer.size(); jx++)
            {
                if(ix==jx)
                    continue;
                if(answer[ix].intersect(answer[jx],p)==QLineF::BoundedIntersection)
                {
                    riMap[ix] = -1;
                    riMap[jx] = -1;
                }
            }
        }




        // 2. Fix inversions and points with no intersections
        // Construct a subset of boundary segments, consisting of the subset between the previous valid ray intersection and the next valid ray intersection
        int chunk_start,chunk_end;
        chunk_start = aux.front();
        chunk_end = aux.back();
        QMap<int,int>::iterator it2;
        QMap<int,QPair<int,int>> subset;
        for (QMap<int,int>::iterator it=riMap.begin(); it!=riMap.end(); ++it)
        {
            if (it.value()!=-1)
                chunk_start = it.value();
            if (it.value()==-1)
            {
                it2= it;

                while(it2.value()==-1&&it2!=riMap.end())
                {
                    ++it2;
                }
                if(it2!=riMap.end())
                    chunk_end = it2.value();
                else
                    chunk_end = seg.lines.size()-1;
                subset.insert(it.key(),qMakePair(chunk_start,chunk_end));
            }
        }




        //For each conflicting ray detected, compute shortest point-to-line (restricted to subset computed above)
        for (QMap<int,QPair<int,int>>::iterator it=subset.begin(); it!=subset.end(); ++it)
        {
            mind = 100000000;
            for (int i=it.value().first; i<=it.value().second; i++)
            {
                dummy = pointToLineDist(rayset[it.key()].p1(),seg.lines[i]);
                d=dummy.length();
                if(d<mind)
                {
                    mind = d;
                    answer[it.key()].setP2(dummy.p2());
                    t = seg.t_at_point(dummy.p2(),i);
                    rtMap.insert(it.key(),t);
                }
            }
        }


        //Smooth t's and extract points
        smooth_t(rtMap,10);


        for (QMap<int,qreal>::iterator it=rtMap.begin(); it!=rtMap.end(); ++it)
        {
            //QTextStream(stdout) << it.key() << "," << it.value() << ", (" << seg.point_at_t(it.value()).rx() << "," << seg.point_at_t(it.value()).ry() << ")\n";
            answer[it.key()].setP2(seg.point_at_t(it.value()));

        }
    }

    return answer;
}

void Stroke::smooth_t(QMap<int,qreal> &map, int iterations) //1-D laplacian smoothing of t
{
    qreal t1,t2;
    for (int i=0; i<iterations; i++)
    {
        for (QMap<int,qreal>::iterator it=map.begin(); it!=map.end(); ++it)
        {
            if(it!=map.begin())
            {
                if(++it==map.end())
                {
                    break;
                }
                --it;          // go back two (because i advanced it above)
                --it;
                t1 = it.value();
                ++it;           // go forward two
                ++it;
                t2 = it.value();
                --it;          // return to position
                it.value() = (t1+t2)/2;
            }
        }
    }
}

QLineF Stroke::pointToLineDist(const QPointF &p, const QLineF &l) //Returns shortest line from point to line segment
{
    // From http://www.fundza.com/vectors/point2line/index.html
    QLineF ans;
    QLineF line_v;
    QLineF pnt_v;

    line_v = l;
    line_v.translate(-l.p1().rx(),-l.p1().ry());

    pnt_v.setPoints(l.p1(),p);
    pnt_v.translate(-l.p1().rx(),-l.p1().ry());

    // Scale both vectors by the length of the line.
    qreal line_len = line_v.length();
    QLineF line_unitv = line_v.unitVector();
    QLineF pnt_v_scaled(0, 0, pnt_v.p2().rx()/line_len, pnt_v.p2().ry()/line_len);

    // Calculate the dot product of the scaled vectors
    qreal t = (line_unitv.dx()*pnt_v_scaled.dx())+(line_unitv.dy()*pnt_v_scaled.dy());


    // Clamp
    if(t<0.0)
        t=0.0;
    else if (t>1.0)
        t=1.0;

    // Scale to T
    QLineF nearest(line_v.p1().rx()*t, line_v.p1().ry()*t, line_v.p2().rx()*t, line_v.p2().ry()*t);
    nearest.translate(l.p1().rx(),l.p1().ry());

    ans.setPoints(p,nearest.p2());
    return ans;
}

qreal Stroke::TV1()
{
    qreal v=0;

    for(int i=1; i<points.size(); i++)
    {
        qreal curr_th = points[i].lRay.length() + points[i].rRay.length();
        qreal prev_th = points[i-1].lRay.length() + points[i-1].rRay.length();

        v += qPow(curr_th/prev_th-1,2);
    }
    v = sqrt(v);

    return v;
}

qreal Stroke::TV2()
{
    QVector<qreal> v;
    qreal v2=0;

    QTextStream(stdout) << "\nThickness Sequence:\n";
    for(int i=1; i<points.size(); i++)
    {
        qreal curr_th = points[i].lRay.length() + points[i].rRay.length();
        qreal prev_th = points[i-1].lRay.length() + points[i-1].rRay.length();
        if (i==1) QTextStream(stdout) << prev_th << ",";
        QTextStream(stdout) << curr_th << ",";
        v.push_back(curr_th/prev_th - 1);
    }


    //second order
    QTextStream(stdout) << "\nTV1 Sequence:\n";
    for(int i=1; i<v.size(); i++)
    {
        if (i==1) QTextStream(stdout) << v[0] << ",";
        QTextStream(stdout) << v[i] << ",";
        v2 += qPow(v[i]/v[i-1]-1,2);

    }

    v2 = sqrt(v2);

    return v2;
}

