#include "Viewer.h"
#include "ui_Viewer.h"

#include <QPainter>
#include <QDebug>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QFileDialog>
#include <QTextStream>
#include <Eigen/Dense>

#include <QString>
#include <QFile>
#include <QStringList>
#include <QLineF>
#include <QtAlgorithms>
#include <QtMath>


#include "libfastmarching.h"
#include "marchingsquares.h"
#include "stroke.h"
#include "boundary.h"
#include "planarcurve.h"

#define SAMPLESIZE 100


Viewer::Viewer(QWidget *parent) : QGLWidget(parent), ui(new Ui::Viewer)
{
    QGLFormat glf = QGLFormat::defaultFormat();
    glf.setSampleBuffers(true);
    this->setFormat(glf);

    ui->setupUi(this);
    setFocusPolicy(Qt::ClickFocus);
}

Viewer::~Viewer()
{
    delete ui;
}

void Viewer::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(),Qt::white);

    for(int i = 0; i < images.size(); i++)
    {
        MyQImage & image = images[i];
        QPoint pos(image.x, image.y);

        painter.translate(pos);
        painter.setOpacity(image.opacity);
        painter.drawImage(0,0, image);
        painter.setOpacity(1.0);

        //MyPolygon & polygon = polys[i];
        //polygon.draw( painter, pos + QPoint(0, this->height()), image.width(), image.height() );

        painter.translate(-pos);
    }

    painter.save();
    painter.translate(2,2);
    painter.setPen(QPen(QColor(0,0,0,128), 30, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPoints(points);
    painter.restore();
    painter.setPen(QPen(Qt::blue, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPoints(points);

    painter.setPen(QPen(Qt::red,3));

    painter.drawPolygon(bndPoly);

    painter.setPen(QPen(Qt::green,1));

    if (!lrays.empty())
    {
        for (auto ray: lrays)
        {
            painter.drawLine(ray);
        }
    }
    if (!rrays.empty())
    {
        for (auto ray: rrays)
        {
            painter.drawLine(ray);
        }
    }

    if(!sPoly.empty())
    {
        painter.setPen(QPen(Qt::cyan,3));
        painter.drawPolygon(sPoly);
    }

}

template<typename QPolygonType>
QPolygonType resamplePolygon(QPolygonType points, int count = 100){
    QPainterPath path;
    path.addPolygon(points);
    auto pathLen = path.length();
    auto stepSize = pathLen / count;
    QPolygonType newPoints;
    for(int i = 0; i < count; i++)
    {
        QPolygon::value_type p = path.pointAtPercent(path.percentAtLength( stepSize * i )).toPoint();
        newPoints << p;
    }
    return newPoints;
}

template<typename QPolygonType>
QPolygonType smoothPolygon( QPolygonType points, int iterations = 1 ){
    for(int i = 0; i < iterations; i++)
    {
        QPolygonType newPoints;

        for(int p = 0; p < points.size(); p++)
        {
            int s = p-1, t = p+1;
            if(s < 0) s = 0;
            if(t > points.size()-1) t = points.size()-1;
            auto prev = points[s];
            auto next = points[t];
            newPoints << (prev + next) * 0.5;
        }

        points = newPoints;
    }
    return points;
}

void Viewer::clearAll()
{
 //   polys.clear();
    images.clear();
    points.clear();
    bnd=NULL;
    bndPoly.clear();
    sPoly.clear();
    strokes.clear();
    //rays.clear();
    lrays.clear();
    rrays.clear();
}

void Viewer::mousePressEvent(QMouseEvent *event)
{
    if(event->buttons().testFlag(Qt::LeftButton))
    {
        if(event->modifiers().testFlag(Qt::ControlModifier))
        {
            auto shape = images.front();

            if(qRed(shape.pixel(event->pos()))) points << event->pos();
        }

        if(event->modifiers().testFlag(Qt::ShiftModifier))
        {
            points.clear();
        }
    }

    update();
}

void Viewer::keyPressEvent(QKeyEvent *event)
{

    if (event->key() == Qt::Key_C) // CLEAR ALL
    {
        clearAll();
    }


    if(event->key() == Qt::Key_I) //LOAD IMAGE
    {
        clearAll();
        images.clear();
        filename = QFileDialog::getOpenFileName(0,"Load image", "", "*.png");
        images << MyQImage(QImage(filename));

        // Convert to double matrix
        img_matrix = Eigen::MatrixXd::Zero(images.front().height(), images.front().width());
        for(int y = 0; y < images.front().height(); y++) {
            for(int x = 0; x < images.front().width(); x++) {
                if(images.front().pixel(x,y) > qRgb(0,0,0))
                {
                    img_matrix(y,x) = 1.0;
                }
            }            
        }

    }

    if(event->key() == Qt::Key_S)
    {
        if(points.size() < 2) return;

        auto shape = images.front();
        auto start_point = points.at(points.size()-2).toPoint();
        auto end_point = points.back().toPoint();

        libfastmarching::Path fm2path;
        libfastmarching::fm2star(shape, start_point, end_point, fm2path);

        // Convert
        QPolygon path;
        for(auto p : fm2path) path << QPoint(p[0], p[1]);

        // Post-process path
        //path = smoothPolygon(resamplePolygon(path,shape.width() * 0.25), 2);        
        path = resamplePolygon(path,SAMPLESIZE);
        path << points.back().toPoint();

        // Make stroke and rays, and save
        Stroke * s = new Stroke(path);        
        strokes.append(s);


        // Visualize
        {
            QImage strokeImage(shape.width(), shape.height(), QImage::Format_ARGB32_Premultiplied);
            strokeImage.fill(Qt::transparent);
            QPainter painter(&strokeImage);
            painter.setRenderHint(QPainter::Antialiasing);

            // Stroke color
            QColor penColor;
            penColor.setHslF(double(rand()) / RAND_MAX * 0.25, 1, 0.5);
            painter.setPen(QPen(penColor, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

            // Draw stroke
            painter.drawPolyline(path);

            images << MyQImage(strokeImage);
        }

    }

    //Extract Boundary
    if(event->key() == Qt::Key_B)
    {
        extractBoundary();
    }


    //Compute Distances toBoundary
    if(event->key() == Qt::Key_D)
    {
        if (strokes.empty())  return;

        if(bnd==NULL) extractBoundary();
        computeDistanceStrokeToBoundary(strokes[0],1);
    }

    //View orthogonal rays
    if(event->key() == Qt::Key_U)
    {
        if (strokes.empty())  return;
        if(bnd==NULL) extractBoundary();
        computeDistanceStrokeToBoundary(strokes[0],0);
    }

    //Compute and Visualize Curvature
    if(event->key() == Qt::Key_K)
    {        
        lrays.clear();
        rrays.clear();
        if(!strokes.empty())
        {
            QVector<qreal> ks = strokes[0]->k_signature();
            lrays = strokes[0]->leftRayset();

            for (int i=0; i<ks.size(); i++)
            {
                lrays[i].setLength(-ks[i]*100); // -k to reverse direction of normal so that it points in correct direction of curvature value, 100 to exaggerate value and visualize
            }
        }
    }

    //Compute and Save all features
    if(event->key() == Qt::Key_F2)
    {
        QString fstring; //features string
        QTextStream out(&fstring);

        //Stroke-to-boundary distance
        if (strokes.empty())  return;
        if(bnd==NULL) extractBoundary();
        computeDistanceStrokeToBoundary(strokes[0],1);

        //Thickness - stroke area/stroke length
        {
            sPoly.clear();
            for (int i=0; i<lrays.size(); i++)
            {
                sPoly << lrays[i].p2();
            }

            for (int i=0; i<rrays.size(); i++)
            {
                sPoly << rrays[i].p2();
            }
            sPoly << lrays[0].p2(); //make closed

            //Compute Area
            {
                qreal a=0;
                for (int i=0; i<sPoly.size(); i++)
                {
                    a+=(sPoly[i].rx()*sPoly[i+1].ry()+sPoly[i].ry()*sPoly[i].rx());
                }
                a = qAbs(a/2);
                QTextStream(stdout) << "Area: " << a << "\n";
                QTextStream(stdout) << "Stroke Length: " << strokes[0]->tot_length << "\n";
                a = a/strokes[0]->tot_length;
                QTextStream(stdout) << "Stroke Thickness (area/length): " << a << "\n";
                out << a << ",";
            }

        }

        //Smoothness - first and second order squared-sum total variation of thickness
        {
            QVector<qreal> v;
            qreal v1=0;
            qreal v2=0;

            for(int i=1; i<lrays.size(); i++)
            {
                int curr_l = i;
                int prev_l = curr_l-1;
                int curr_r = (rrays.size()-1)-i;
                int prev_r = curr_r+1;

                qreal curr_th = lrays[curr_l].length() + rrays[curr_r].length();
                qreal prev_th = lrays[prev_l].length() + rrays[prev_r].length();

                v.push_back(qPow(curr_th/prev_th-1,2));
                v1 += qPow(curr_th/prev_th-1,2);

            }
            v1 = sqrt(v1);

            //second order
            for(int i=1; i<v.size(); i++)
            {
                v2 += qPow(v[i]-v[i-1],2);
            }

            v2 = sqrt(v2);

            QTextStream(stdout) << "First Order Stroke Smoothness: " << v1 << "\n";
            QTextStream(stdout) << "Second Order Stroke Smoothness: " << v2 << "\n";

            out << v1 << ",";
            out << v2;
        }

        //Curvature
        {
            QVector<qreal> curvature = strokes[0]->k_signature();

            //Curviness - 20 values, i.e. every 5 points
            qreal t=0;
            for (int i=0; i<curvature.size(); i++)
            {
                t+= curvature[i];
                if((i!=0)&&(i%5==0))
                {
                    out << t << ",";
                    QTextStream(stdout) << "k "<< i << ":" << t << "\n";

                    t=0;
                    t+= curvature[i];
                }
            }
        }

        saveFeatures(fstring);
        saveStroke_and_Boundary();

    }




    update();
}

bool Viewer::saveStroke_and_Boundary()
{
    QString filesave = filename;
    filesave.replace(QString(".png"),QString("stroke.txt"));
    QFile file(filesave);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        for (int i=0; i<strokes.size(); i++)
        {
            QPolygonF sp = strokes[i]->path;
            out << "Stroke " << i << "\n";
            for(auto p:sp)
                out << p.rx() << "," << p.ry() << "\n";
        }

        file.close();
        return 1;
    }
    return 0;

}

bool Viewer::saveFeatures(QString s)
{
    QString filesave = filename;
    filesave.replace(QString(".png"),QString(".txt"));
    QFile file(filesave);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        out << s;
        file.close();

        return 1;
    }
    return 0;
}

void Viewer::computeDistanceStrokeToBoundary(Stroke * s, bool fix) //Wrapper, computes stroke-to-boundary distance
{
    QLineF r;
    int i1,i2;

    lrays.clear();
    rrays.clear();

    // Compute splice points
    r = s->rayStart();
    i1 = bnd->closestIntersection(r).first;

    r = s->rayFinish();
    i2 = bnd->closestIntersection(r).first;

    // Split boundary
    segment1 = bnd->getSegment(i1,i2);
    segment2 = bnd->getSegment(i2,i1);

    lrays = parametrizedBoundaryIntersection(segment1,strokes[0]->leftRayset(),fix);
    rrays = parametrizedBoundaryIntersection(segment2,strokes[0]->rightRayset(),fix);
}

QVector<QLineF> Viewer::parametrizedBoundaryIntersection(PlanarCurve seg, QVector<QLineF> rayset, bool fix) // Computes stroke-to-boundary distance for either left or right side separately
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

        for (auto it=rtMap.begin(); it!=rtMap.end(); ++it)
        {
            answer[it.key()].setP2(seg.point_at_t(it.value()));
        }
    }

    return answer;
}

void Viewer::smooth_t(QMap<int,qreal> &map, int iterations) //1-D laplacian smoothing of t
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

QLineF Viewer::pointToLineDist(const QPointF &p, const QLineF &l) //Returns shortest line from point to line segment
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

void Viewer::extractBoundary()
{
    if(bnd==NULL)
    {
        QPolygonF b;
        for(auto pixel : MarchingSquares(img_matrix,1.0).march())
            b << QPointF(pixel.x(),pixel.y());

        //b = resamplePolygon(b,100);

        bnd = new Boundary(b);
        bndPoly = bnd->getPolygon();
    }
}
