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

#include "png2poly.h"
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

    if (!bndPolys.empty())
    {
        for (auto p : bndPolys)
        {
            painter.drawPolygon(p);
        }
    }    

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
    images.clear();
    points.clear();

    sPoly.clear();
    strokes.clear();

    lrays.clear();
    rrays.clear();

    bnds.clear();
    bndPolys.clear();
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

    //Extract Boundary(ies)
    if(event->key() == Qt::Key_B)
    {
        extractBoundary();
    }


    //Compute Distances toBoundary(ies)
    if(event->key() == Qt::Key_D)
    {
        if (strokes.empty())    return;             // Nothing to compute
        if(bnds.empty())        extractBoundary();  // Extract Boundary

        mapStrokeToBoundary();                      // Ensure each stroke is assigned to a boundary

        lrays.clear();
        rrays.clear();

        for (int i=0; i<strokes.size(); i++) {
            strokes[i]->computeDistanceStrokeToBoundary(1);
            lrays << strokes[i]->leftRayset();
            rrays << strokes[i]->rightRayset();
        }
    }

    //View orthogonal rays
    if(event->key() == Qt::Key_U)
    {
        if (strokes.empty())    return;             // Nothing to computeu
        if(bnds.empty())        extractBoundary();  // Extract Boundary

        mapStrokeToBoundary();                      // Ensure each stroke is assigned to a boundary

        lrays.clear();
        rrays.clear();

        for (int i=0; i<strokes.size(); i++) {
            strokes[i]->computeDistanceStrokeToBoundary(0);
            lrays << strokes[i]->leftRayset();
            rrays << strokes[i]->rightRayset();
        }


    }

    //Compute and Visualize Curvature
    if(event->key() == Qt::Key_K)
    {
        lrays.clear();
        rrays.clear();
        if(!strokes.empty())
        {
            for (auto s : strokes) {
                QVector<qreal> ks = s->k_signature();
                QVector<QLineF> lines = s->leftOrthos();
                //lrays = strokes[0]->leftRayset();

                for (int i=0; i<ks.size(); i++)
                {
                    lines[i].setLength(-ks[i]*100); // -k to reverse direction of normal so that it points in correct direction of curvature value, 100 to exaggerate value and visualize
                }

                lrays << lines;
            }
        }
    }

    if(event->key() == Qt::Key_0) //load stroke hpgl
    {
        //clearAll();
        //images.clear();

        QString fstring; //features string
        QTextStream out(&fstring);

        filename = QFileDialog::getOpenFileName(0,"Load hpgl", "", "*.csv");
        QFile file(filename);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return;

        QPolygonF path;

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList list = line.split(",");
            path << QPointF(list[0].toDouble(),list[1].toDouble());
        }

        path = resamplePolygon(path,101);
        Stroke * s = new Stroke(path);
        strokes.append(s);

        sPoly = path;

        // Boundary
        if (bnds.empty())
        {
            filename = QFileDialog::getOpenFileName(0,"Load hpgl", "", "*.csv");
            QFile file(filename);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return;

            bnds.clear();
            bndPolys.clear();

            QPolygonF aux;

            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                QStringList list = line.split(",");
                aux << QPointF(list[0].toDouble(),list[1].toDouble());
            }

            //uuuaux = resamplePolygon(aux,1000);
            bnds.push_back(new Boundary(aux));
            bndPolys.push_back(bnds.front()->getPolygon());

        }

    }

    //Save strokes
    if(event->key() == Qt::Key_F3) {

        if (!strokes.empty()) {
            saveStroke_and_Boundary();
        }

    }


    //Compute and Save all features
    if(event->key() == Qt::Key_F2)
    {

        if (strokes.empty())    return;             // Nothing to compute
        if(bnds.empty())        extractBoundary();  // Extract Boundary

        mapStrokeToBoundary();                      // Ensure each stroke is assigned to a boundary

        QString fstring; //features string
        QTextStream out(&fstring);

        // Compute Stroke-to-boundary distances
        lrays.clear();
        rrays.clear();

        for (int i=0; i<strokes.size(); i++) {
            strokes[i]->computeDistanceStrokeToBoundary(1);
            lrays << strokes[i]->leftRayset();
            rrays << strokes[i]->rightRayset();
        }

        // FOR EACH STROKE....
        for (int i=0; i<strokes.size(); i++) {
            QTextStream(stdout) << "STROKE " << i << "=======\n";
            QString sFeat;
            QTextStream sOut(&sFeat);

            //Thickness - stroke area/stroke length
            //sOut << strokes[i]->thickness()/strokes[0]->thickness() << ","; //MANY STROKES
            sOut << strokes[i]->thickness() << ",";                             //ONE STROKE

            QTextStream(stdout) << "Thickness: " << strokes[i]->thickness() << "\n";
            //QTextStream(stdout) << "Relative Thickness: " << strokes[i]->thickness()/strokes[0]->thickness() << "\n";

            //Smoothness
            sOut << strokes[i]->TV1() << ",";
            //sOut << strokes[i]->TV2() << ",";

            QTextStream(stdout) << "TV1: " << strokes[i]->TV1() << "\n";
            //QTextStream(stdout) << "TV2: " << strokes[i]->TV2() << "\n";


            //Curvature
            QVector<qreal> curvature = strokes[i]->k_signature();

            //Curviness - 20 values, i.e. every 5 points
            qreal t=0;
            for (int j=0; j<curvature.size(); j++)
            {
                t+= curvature[j];
                if((j!=0)&&(j%5==0))
                {
                    sOut << t << ",";

                    QTextStream(stdout) << "k "<< j << ":" << t << "\n";
                    t=0;
                    t+= curvature[i];
                }
            }
            out << sFeat;
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
            out << "##Stroke " << i << "\n";
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
    QString filesave = QFileDialog::getSaveFileName(0,"Save Featires", "", "*.txt");

//    QString filesave = filename;
//    filesave.replace(QString(".png"),QString(".txt"));
//    filesave.replace(QString(".plt.csv"),QString(".txt"));
    QFile file(filesave);

    if (s.endsWith(",")) {
        s.chop(1);
    }


    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        out << s;
        file.close();

        return 1;
    }
    return 0;
}

void Viewer::extractBoundary()
{    
    if (bnds.empty())
    {
        bnds.clear();
        bndPolys.clear();
        QVector<png2poly::poly> ans;
        ans = png2poly(img_matrix).mask2poly();

        for (auto p : ans)
            bnds.push_back(new Boundary(p.p));

        for (auto b : bnds)
            bndPolys.push_back(b->getPolygon());

        QTextStream(stdout) << "Number of Boundaries:" << bnds.size() << "\n";

    }
}

void Viewer::mapStrokeToBoundary()
{    
    for (int i=0; i<strokes.size(); i++) {
        for (int j=0; j<bnds.size(); j++) {
            QPolygonF b = bnds[j]->getPolygon();
            if (b.containsPoint(strokes[i]->start(),Qt::OddEvenFill)) {
                QTextStream(stdout) << "Stroke " << i << " is in boundary " << j << "\n";
                strokes[i]->bnd = bnds[j];
            }
        }
    }
}
