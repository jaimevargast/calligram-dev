#include <math.h>
#define _USE_MATH_DEFINES

#include "Viewer.h"
#include "ui_Viewer.h"
#include <QPainter>
#include <QKeyEvent>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <iostream>

using namespace std;

// Eigen library
#include "../Eigen/Core"
#include "../Eigen/Geometry"
using namespace Eigen;
typedef Vector2d Vector2;

#define radiansToDegrees(angleRadians) (angleRadians * 180.0 / M_PI)

// Intersection library
#include "intersections.h"

// Helper functions:
QPolygonF resamplePolygon(QPolygonF points, int count = 100){
	QPainterPath path;
	path.addPolygon(points);
	auto pathLen = path.length();
	auto stepSize = pathLen / count;
	QPolygonF newPoints;
	for(int i = 0; i < count; i++)
		newPoints << path.pointAtPercent(path.percentAtLength( stepSize * i ));
	newPoints << path.pointAtPercent(0);
	return newPoints;
}
QPolygonF smoothPolygon( QPolygonF points, int iterations){
	for(int i = 0; i < iterations; i++)
	{
		QPolygonF newPoints;

		for(int p = 0; p < points.size(); p++)
		{
			int s = p-1, t = p+1;
			if(s < 0) s = 0;
			if(t > points.size()-1) t = points.size() - 1;
			QPointF prev = points[s];
			QPointF next = points[t];
			newPoints << (prev + next) * 0.5;
		}

		points = newPoints;
	}
	return points;
}

bool isIntersect( const QLineF & line1, const QPainterPath & path, QPointF & isect, double & t )
{
	QPolygonF polygon = path.toFillPolygon();

	QMap<double, QPointF> isections;

    size_t N = polygon.size();

    for(size_t i = 0; i < N; i++)
	{
		int j = i + 1;
		if(i == polygon.size()-1) j = 0;

		QLineF line2( polygon[i], polygon[j] );

		//intersection2D::Point isect1, isect2;
		//intersection2D::Segment s1();
		//intersection2D::Segment s2();
		//int isIntersect = intersection2D::intersect2D_2Segments( s1, s2, &isect1, &isect2 );

		float x, y;

		int isIntersect = get_line_intersection(line1.x1(), line1.y1(), line1.x2(), line1.y2(), 
												line2.x1(), line2.y1(), line2.x2(), line2.y2(), &x, &y);

		if( isIntersect > 0 )
		{
			QPointF cur_isect = QPointF( x, y );
			QPointF delta = (cur_isect - line1.p1());
			double distance = Vector2(delta.x(), delta.y()).norm();

            t = double(i+1) / double (N-1);
			isections[ distance ] = cur_isect;
		}
	}

	if( isections.empty() )
		return false;
	else
	{
        isect =  isections[ isections.keys().front() ];
		return true;
	}
}

double signedAngle( Vector2 A, Vector2 B ){
	double cross = (A.x()*B.y()) - (A.y()*B.x());
	double dot = A.dot(B) ;
	double angle = atan2( cross, dot );
	return angle;
}

double signedAngleAsVectors(QLineF l1, QLineF l2)
{
	Vector2 A( l1.dx(), l1.dy() );
	Vector2 B( l2.dx(), l2.dy() );

	if(A.squaredNorm() == 0 || B.squaredNorm() == 0) return 0;

	A.normalize();
	B.normalize();

	return signedAngle( A, B );
}

double projectPointSegment( QPointF point, QLineF line )
{
	Vector2 p(point.x(), point.y());
	Vector2 v(line.p1().x(), line.p1().y());
	Vector2 w(line.p2().x(), line.p2().y());
	double l2 = (w-v).squaredNorm();
	return l2 > 0 ? (p-v).dot(w-v) / l2 : 0;
}

void distanceToSegment( QPointF point, QLineF line, double & distance, double & t, QPointF & projection )
{
	t = projectPointSegment(point, line);
	projection = line.pointAt(t);

	if(t < 0)		distance = QLineF(point, line.p1()).length(); // 'left' of line
	else if(t > 0)	distance = QLineF(point, line.p2()).length(); // 'right' of line
	else			distance = QLineF(point, projection).length(); // 'between' line
}

void distanceAngleTo( QPointF point, QLineF line, double & distance, double & angle )
{
	double t = 0;
	QPointF projection;

	distanceToSegment(point, line, distance, t, projection);

	double x = signedAngleAsVectors( line, QLineF(point, line.p1()) );
	//angle = radiansToDegrees(x);
	angle = 90;
}

Viewer::Viewer(QWidget *parent) : QWidget(parent), ui(new Ui::Viewer)
{
	ui->setupUi(this);

	setFocusPolicy(Qt::ClickFocus);

	QString defaultFile = "../points.txt";

	QPolygonF points;

	// Load points from file
	if(QFileInfo(defaultFile).exists()){
		QFile file( defaultFile );
		file.open(QFile::ReadOnly | QFile::Text);
		QTextStream in(&file);
		QStringList lines = in.readAll().split('\n');
		for( auto line : lines ){
			auto pointCoord = line.split(",");
			if(pointCoord.size() < 2) continue;
			points << QPointF( pointCoord[0].toDouble(), pointCoord[1].toDouble() );
		}
		points << points.first();
	}
	else
		return;

	QPainterPath contour;
	bool isFirst = true;

	for(auto p : points)
	{
		float x = p.x();
		float y = p.y();

		if(isFirst) {
			contour.moveTo(QPoint(x,y));
			isFirst = false;
		}

		contour.lineTo(QPoint(x,y));
	}
	contours.push_back(contour);
}

Viewer::~Viewer()
{
	delete ui;
}

void Viewer::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
    //painter.setRenderHint(QPainter::Antialiasing);
	painter.fillRect(rect(), Qt::white);

	if(contours.empty()) return;

	// Draw contour
	painter.setPen(QPen(Qt::green, 4));
    for (auto & contour : contours) painter.drawPath(contour);

	if(paths.empty()) return;

	// Draw user path
	painter.setPen(QPen(Qt::green, 4));
	for (auto & path : paths) painter.drawPolyline(path);

    // Find starting and ending point on the contour
    QPainterPath path1;
    QPointF isect_start, isect_end;
    double t_s = 0.0, t_e = 0.5;
    for (auto & poly : paths)
    {
        path1.addPolygon(poly);
        QPointF path_start0 = path1.pointAtPercent(0);
        QPointF path_start1 = path1.pointAtPercent(0.001);

        QPointF path_end0 = path1.pointAtPercent(0.999);
        QPointF path_end1 = path1.pointAtPercent(1);

        Vector2 p_start(path_start0.x(),path_start0.y()), q_start(path_start1.x(),path_start1.y());
        Vector2 p_end(path_end0.x(),path_end0.y()), q_end(path_end1.x(),path_end1.y());

        Vector2 t_start( (q_start-p_start).normalized());
        Vector2 t_end( (q_end-p_end).normalized());

        //q_start = p_start + t_start * (p_start-q_start).norm();
        //q_end = p_end + t_end * (p_end-q_end).norm();

        Vector2 u1 = p_start - t_start * 200;
        Vector2 u2 = p_end + t_end * 200;

        painter.setPen(QPen(QColor(255,255,0,80), 2));
        painter.drawLine( p_start.x(), p_start.y(), u1.x(), u1.y());
        painter.drawLine( p_end.x(), p_end.y(), u2.x(), u2.y());

        QPainterPath cont = contours.front();
        QLineF raySegment1( p_start.x(), p_start.y(), u1.x(), u1.y() );
        QLineF raySegment2( p_end.x(), p_end.y(), u2.x(), u2.y() );

        if( isIntersect(raySegment1, cont, isect_start, t_s) )
        {
            painter.setPen(QPen(Qt::red, 5));
            painter.drawPoint( isect_start );
        }

        if( isIntersect(raySegment2, cont, isect_end, t_e) )
        {
            painter.setPen(QPen(Qt::green, 5));
            painter.drawPoint( isect_end );
        }
    }

	int pointSize = 4;

    // Sample the contour
    QVector<QPointF> cp1, cp2, cq, cm, pp, pq, pm;

    int samplesCount = 100;

    QPolygonF tangents;

    bool reverse = false;
    if(t_e < t_s)
    {
        swap(t_e,t_s);
        reverse = true;
    }

    for (auto & contour : contours)
    {
        double stepSize = (t_e - t_s) / double(samplesCount);
        for(int i = 0; i <= samplesCount; i++)
        {
            double thisStep = t_s + stepSize * i;
            if (thisStep > 1)
            {
                thisStep = thisStep - 1;
            }
            // Current point
            QPointF pf = contour.pointAtPercent(thisStep);
            cp1.push_back(pf);
        }
        painter.setPen(QPen(Qt::red, pointSize));
        for (int i = 0; i < samplesCount; ++i)
        {
            painter.drawPoint(cp1[i]);
        }

        stepSize = (t_s + 1 - t_e) / double(samplesCount);
        for(int i = 0; i <= samplesCount; i++)
        {
            double thisStep = t_e + stepSize * i;
            if (thisStep > 1)
            {
                thisStep = thisStep - 1;
            }
            // Current point
            QPointF pf = contour.pointAtPercent(thisStep);
            cp2.push_back(pf);
        }
        painter.setPen(QPen(Qt::yellow, pointSize));
        for (int i = 0; i < samplesCount; ++i)
        {
            painter.drawPoint(cp2[i]);
        }
    }

    // Sample the path drawn by user
    QPainterPath path;
    double stepSize;
    for (auto & poly : paths)
    {
        path.addPolygon(poly);
        auto pathLen = path.length();
        stepSize = pathLen / samplesCount;
        for(int i = 0; i <= samplesCount; i++)
        {
            // Current point
            QPointF p = path.pointAtPercent(path.percentAtLength( stepSize * i ));
            pp.push_back(p);

            // Next point
            if(i < (samplesCount) -1)
            {
                QPointF q = path.pointAtPercent(path.percentAtLength( stepSize * (i+1) ));
                pq.push_back(q);
            }
            else
            {
				// For last tangent
                QPointF m = path.pointAtPercent(path.percentAtLength( stepSize * (i-1) ));
                QPointF q = (1 * (p-m)) + p;
                pm.push_back(m);
                pq.pop_back();
                pq.push_back(q);
            }
        }
        painter.setPen(QPen(Qt::blue, pointSize));
        for (int i = 0; i < samplesCount; ++i)
        {
            painter.drawPoint(pp[i]);
        }
    }

    // Store the angle and length in the vector
	int x = 100;
	int graphWidth = 400;
	int segmentWidth = graphWidth / samplesCount;

	QPointF graphStart( 500, 200 );
	painter.translate( graphStart );
	painter.setOpacity( 0.6 );

    for(int i = 0; i < samplesCount; i++)
    {
        //if (reverse == false)
        {
			auto pA = pp[i], pB = pp[i+1];
			auto topA = cp1[i], topB = cp1[i+1];
			auto bottomA = cp2[i], bottomB = cp2[i+1];
			
			QLineF segment(pA, pB);

			double distanceA = 0, angleA = 0;
			double distanceB = 0, angleB = 0;

			// Top:
			distanceAngleTo(topA, segment, distanceA, angleA);
			angles.push_back(angleA);
			lengths.push_back(distanceA);

			distanceAngleTo(topB, segment, distanceB, angleB);
			angles.push_back(angleB);
			lengths.push_back(distanceB);

			// Debug:
			{
				painter.setPen(QPen(Qt::red,1));
				QLineF l1(0,0,distanceA*0.5,0);
				l1.setAngle(angleA);
				painter.drawLine(l1.translated(x,0));
				QLineF l2(0,0,distanceB*0.5,0);
				l2.setAngle(angleB);
				painter.drawLine(l2.translated(x+segmentWidth*0.5,0));
			}

			// Bottom:
			distanceAngleTo(bottomA, segment, distanceA, angleA);
			angles.push_back(angleA);
			lengths.push_back(distanceA);

			distanceAngleTo(bottomB, segment, distanceB, angleB);
			angles.push_back(angleB);
			lengths.push_back(distanceB);

			// Debug:
			{
				painter.setPen(QPen(Qt::yellow,1));
				QLineF l1(0,0,distanceA*0.5,0);
				l1.setAngle(angleA);
				painter.drawLine(l1.translated(x,0));
				QLineF l2(0,0,distanceB*0.5,0);
				l2.setAngle(angleB);
				painter.drawLine(l2.translated(x+segmentWidth*0.5,0));
			}

			x += segmentWidth;
        }
    }

	//qDebug() << angles;
}

void Viewer::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Space)
	{
		reduceDimention();
	}
}

void Viewer::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		lastPoint = event->pos();
		scribbling = true;

		QPolygonF path;
		paths.push_back(path);
	}
	else
	{
		paths.clear();
		update();
	}
}
void Viewer::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && scribbling) {
		scribbling = false;

		if(!paths.empty())
		{
			auto & path = paths.back();
			path = smoothPolygon(path, 10);
		}
	}

	update();
}

void Viewer::mouseMoveEvent(QMouseEvent *event)
{
	if ((event->buttons() & Qt::LeftButton) && scribbling)
	{
		int prev_y = lastPoint.y();
		int prev_x = lastPoint.x();

		int y = event->pos().y();
		int x = event->pos().x();

		auto & p = paths.back();

		p << QPoint(x, y);

		update();
	}
}

void Viewer::reduceDimention()
{
    /*
        for (auto & polygon : paths)
        {
            QVector<double> distancesTop, distancesBottom;

            QPainterPath path;
            path.addPolygon(polygon);

            int samplesCount = 60;

            auto pathLen = path.length();
            auto stepSize = pathLen / samplesCount;

            QPolygonF tangents;

            int pointSize = 4;
            int lineWidth = 2;

            int outside = 200;

            for(int i = 0; i <= samplesCount; i++)
            {
                QPointF pf, qf, mf;

                // Current point
                pf = path.pointAtPercent(path.percentAtLength( stepSize * i ));

                // Next point
                if(i < samplesCount-1)
                {
                    qf = path.pointAtPercent(path.percentAtLength( stepSize * (i+1) ));
                }
                else
                {
                    mf = path.pointAtPercent(path.percentAtLength( stepSize * (i-1) ));
                    qf = (1 * (pf-mf)) + pf;
                }

                painter.setPen(QPen(Qt::red, pointSize));
                painter.drawPoint(pf);

                Vector2 p(pf.x(),pf.y()), q(qf.x(),qf.y());

                if( (q-p).norm() < 1e-5 ) continue; // Segment too short

                Vector2 t( (q-p).normalized() );

                q = p + t * (p-q).norm();

                painter.setPen(QPen(Qt::blue, lineWidth * 0.5));
                painter.drawLine( p.x(), p.y(), q.x(), q.y() );

                Rotation2D<double> rot( M_PI_2 );
                Vector2 w = rot * t;
                Vector2 u1 = p + w * outside;
                Vector2 u2 = p + w * -outside;

                painter.setPen(QPen(QColor(255,255,0,80), lineWidth));
                painter.drawLine( p.x(), p.y(), u1.x(), u1.y());
                painter.drawLine( p.x(), p.y(), u2.x(), u2.y());

                QPainterPath cont = contours.front();
                QPointF isect;
                QLineF raySegment1( p.x(), p.y(), u1.x(), u1.y() );
                QLineF raySegment2( p.x(), p.y(), u2.x(), u2.y() );

                if( isIntersect(raySegment1, cont, isect) )
                {
                    painter.setPen(QPen(Qt::red, 5));
                    painter.drawPoint( isect );

                    distancesBottom.push_back( (p - Vector2(isect.x(), isect.y())).norm() );
                }

                if( isIntersect(raySegment2, cont, isect) )
                {
                    painter.setPen(QPen(Qt::green, 5));
                    painter.drawPoint( isect );

                    distancesTop.push_back( (p - Vector2(isect.x(), isect.y())).norm() );
                }
            }

            // Draw the function
            QPainterPath f1, f2;

            int width = 300;
            int height = 100;

            double minf1 = *std::min_element(distancesBottom.begin(), distancesBottom.end());
            double minf2 = *std::min_element(distancesTop.begin(), distancesTop.end());

            double maxf1 = *std::max_element(distancesBottom.begin(), distancesBottom.end());
            double maxf2 = *std::max_element(distancesTop.begin(), distancesTop.end());

            QLineF l1(50,50,50+width,50);
            QLineF l2 = l1.translated( QPointF(width,0) );

            f1.moveTo(l1.p1());
            f2.moveTo(l2.p1());

            size_t N = std::min(distancesTop.size(), distancesBottom.size());

            for(size_t i = 0; i < N; i++)
            {
                double t = double(i) / (distancesTop.size()-1);

                // Normalized
                //double v1 = (distancesTop[i] - minf1) / (maxf1-minf1);
                //double v2 = (distancesBottom[i] - minf2) / (maxf2-minf2);

                double v1 = distancesTop[i] / 50;
                double v2 = distancesBottom[i] / 50;

                QPointF p1 = l1.pointAt(t);
                QPointF p2 = l2.pointAt(t);

                f1.lineTo( p1 - QPoint(0,v1 * height)  );
                f2.lineTo( p2 - QPoint(0,v2 * height)  );
            }

            painter.translate(0, 600);

            painter.setPen(QPen(Qt::green, 2));
            painter.drawPath(f1);

            painter.setPen(QPen(Qt::red, 2));
            painter.drawPath(f2);
        }*/

}
