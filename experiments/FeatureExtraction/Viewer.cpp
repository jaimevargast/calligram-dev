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


#include "libfastmarching.h"
#include "marchingsquares.h"
#include "stroke.h"
#include "boundary.h"
#include "planarcurve.h"



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

    if (!rays.empty())
    {
        for (auto ray: rays)
        {
            painter.drawLine(ray);
        }
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
    strokes.clear();
    rays.clear();
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
        images << MyQImage(QImage(QFileDialog::getOpenFileName(0,"Load image", "", "*.png")));

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
        path = smoothPolygon(resamplePolygon(path,100),2);

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


    //Compute Distances toBoundary
    if(event->key() == Qt::Key_D)
    {
        computeDistanceStrokeToBoundary(strokes[0],1);
    }

    if(event->key() == Qt::Key_U)
    {
        computeDistanceStrokeToBoundary(strokes[0],0);
    }

    if(event->key() == Qt::Key_K) // Compute and Visualize Curvature
    {
        rays.clear();
        if(!strokes.empty())
        {
            QVector<qreal> ks = strokes[0]->k_signature();
            rays = strokes[0]->leftRayset();

            for (int i=0; i<ks.size(); i++)
            {
                rays[i].setLength(-ks[i]*100); // -k to reverse direction of normal so that it points in correct direction of curvature value, 100 to exaggerate value and visualize
            }
        }
    }


    //SAVE BOUNDARY AND STROKES
//    if(event->key() == Qt::Key_F2)
//    {
//        QFile file("strokeandpath.txt");
//        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
//        {


//            QTextStream out(&file);
//            out << "Boundary: \n";
//            for (int ix=0; ix<boundary.size(); ix++)
//            {
//                out << boundary[ix].rx() << "," << boundary[ix].ry() << "\n";
//            }
//            out << "Stroke: \n";
//            for (int ix=0; ix<strokes.size(); ix++)
//            {
//                out << "Stroke " << ix << ":\n";
//                for (int jx=0; jx<strokes[ix].size(); jx++)
//                {
//                    out << strokes[ix][jx].rx() << "," << strokes[ix][jx].ry() << "\n";
//                }
//            }

//            file.close();
//        }
//    }


    update();
}

void Viewer::computeDistanceStrokeToBoundary(Stroke * s, bool fix)
{
    QPair<int,QPointF> ans;
    QVector<QPair<int,QPointF>> ps;
    ps.clear();
    rays.clear();

    // Compute splice points
    int i1,i2;
    QLineF r = s->rayStart();
    i1 = bnd->closestIntersection(r).first;

    r = s->rayFinish();
    i2 = bnd->closestIntersection(r).first;

    // Split boundary
    segment1 = bnd->getSegment(i1,i2);
    segment2 = bnd->getSegment(i2,i1);

    rays = parametrizedBoundaryIntersection(segment1,strokes[0]->leftRayset(),fix);
    rays += parametrizedBoundaryIntersection(segment2,strokes[0]->rightRayset(),fix);
}

QVector<QLineF> Viewer::parametrizedBoundaryIntersection(PlanarCurve seg, QVector<QLineF> rayset, bool fix)
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


QLineF Viewer::pointToLineDist(const QPointF &p, const QLineF &l)
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
