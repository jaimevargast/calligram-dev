//#pragma once

#include <Eigen/Core>
#include <Eigen/Sparse>
#include <QVector>
#include <QPolygonF>
#include <QPair>
#include <QTextStream>
#include <vector>

typedef Eigen::Triplet<int> T;

struct png2poly
{
    struct poly
    {
        QPolygonF p;
        bool            hole;
    };

    Eigen::MatrixXd image;

    png2poly(const Eigen::MatrixXd & image) : image(image)
    {
    }

//    QPolygonF prueba()
//    {
//        QPolygonF ans;
//        ans << QPointF(10,10) << QPointF(30,10) << QPointF(20,30);
//        return ans;
//    }

    QVector<poly> mask2poly()
    {
        QVector<poly> ans;
        Eigen::MatrixXd mask = image;
        Eigen::MatrixXd maskRef;

        maskRef.setZero(mask.rows()+2,mask.cols()+2);

//         DEBUGGING ****************************************/
//        for(int i=0; i<image.rows(); i++)
//        {
//            for (int j=0; j<image.cols(); j++)
//            {
//                QTextStream(stdout) << image(i,j);
//            }
//            QTextStream(stdout) << "\n";
//        }

        /*DEBUGGING ****************************************
        for(int i=0; i<mask.rows(); i++)
        {
            for (int j=0; j<mask.cols(); j++)
            {
                QTextStream(stdout) << mask(i,j);
            }
            QTextStream(stdout) << "\n";
        }
        /****************************************************/

        for (int ix=0; ix<mask.rows(); ix++) {
            for (int jx=0; jx<mask.cols(); jx++) {
                maskRef(ix+1,jx+1) = mask(ix,jx);
            }
        }

        mask = maskRef;

        /* DEBUGGING ****************************************
        QTextStream(stdout) << "mask rows " << mask.rows() << " cols " << mask.cols() << "\n";
        for(int i=0; i<mask.rows(); i++)
        {
            for (int j=0; j<mask.cols(); j++)
            {
                QTextStream(stdout) << mask(i,j);
            }
            QTextStream(stdout) << "\n";
        }
        /*****************************************************/

        QVector<QVector<int>> indices = findNonZero(mask);

        //  position of segments on the edge for first dimension
        Eigen::SparseMatrix<int> seg1(mask.rows()+1,mask.cols());

        // position of segments on the edge for second dimension
        Eigen::SparseMatrix<int> seg2(mask.rows(),mask.cols()+1);

        /*********************************************************
        QTextStream(stdout) << "===== Non Zero elements =====\n";
        /*********************************************************/

        std::vector<T> tseg1;
        std::vector<T> tseg2;

        for (int ix=0; ix<indices[0].size(); ix++)
        {
            /**************************************
            QTextStream(stdout) << indices[0].at(ix) << "," << indices[1].at(ix) << "\n";
            /************************************/

            int ind1 = indices[0].at(ix);
            int ind2 = indices[1].at(ix);

            if (mask(ind1-1,ind2)==0)           // above
                //seg1.insert(ind1,ind2) = 1;
                tseg1.push_back(T(ind1,ind2,1));

            if (mask(ind1+1,ind2)==0)           // below
                //seg1.insert(ind1+1,ind2)=1;
                tseg1.push_back(T(ind1+1,ind2,1));

            if (mask(ind1,ind2-1)==0)           // left
                //seg2.insert(ind1,ind2) = 1;
                tseg2.push_back(T(ind1,ind2,1));

            if (mask(ind1,ind2+1)==0)           // right
                //seg2.insert(ind1,ind2+1) = 1;
                tseg2.push_back(T(ind1,ind2+1,1));
        }

        seg1.setFromTriplets(tseg1.begin(),tseg1.end());
        seg2.setFromTriplets(tseg2.begin(),tseg2.end());

        /**********************************************************
        QTextStream(stdout) << "===== SPARSE seg 1 =====\n";
        indices = findNonZero(seg1);
        for (int ix=0; ix<indices[0].size(); ix++)
        {
            QTextStream(stdout) << indices[0].at(ix) << "," << indices[1].at(ix) << "\n";
        }
        QTextStream(stdout) << "===== SPARSE seg 2 =====\n";
        indices = findNonZero(seg2);
        for (int ix=0; ix<indices[0].size(); ix++)
        {
            QTextStream(stdout) << indices[0].at(ix) << "," << indices[1].at(ix) << "\n";
        }

        QTextStream(stdout) << "non-zero elements in seg 1: " << seg1.nonZeros() << "\n";
        QTextStream(stdout) << "non-zero elements in seg 2: " << seg2.nonZeros() << "\n";
        /*****************************************************************/


        QPolygonF curve;
        while (seg1.nonZeros()>1)
        {
            curve = chainSeg(seg1,seg2);
            if (!curve.empty())
            {
                poly t;
                t.p = curve;
                t.hole = true;
                ans.push_back(t);
            }
        }

        ans[0].hole=false;
        return ans;
    }

    //Chaining of segments
    QPolygonF chainSeg(Eigen::SparseMatrix<int> &seg1, Eigen::SparseMatrix<int> &seg2)
    {

        QPolygonF curve;

        //choose one start segment
        QVector<QVector<int>> indices = findNonZero(seg1);
        int ind1=indices[0].at(0);
        int ind2=indices[1].at(0);

        /***
        QTextStream(stdout) << "chainSeg Iteration ====================\n";
        QTextStream(stdout) << "ind1: " << ind1 << " ind 2: " << ind2 << "\n";
        /***/

        curve << QPointF(ind2,ind1);
        curve << QPointF(ind2+1,ind1);
        QTextStream(stdout) << "<<" << ind1 << "," << ind2 << "\n";
        QTextStream(stdout) << "<<" << ind1 << "," << ind2+1 << "\n";


        seg1.coeffRef(ind1,ind2) = 0;
        seg1.prune(0,0);


        /*% pos is a variable to remember in which direction we are
              % progressing
              % [1 1] if we're going in a growing index number
              % [-1 0] otherwise*/
        QPair<int,int> pos = qMakePair(1,1);

        // precise if the last added segment comes from seg1 or seg2
        bool last1 = true;


        while(true)
        {
            if(last1==true) //the last segment was from dimension 1
            {
                if (seg1.coeff(ind1,ind2+pos.first))
                {
                    ind2 = ind2+pos.first;
                    seg1.coeffRef(ind1,ind2)=0;
                    seg1.prune(0,0);
                    curve << QPointF(ind2+pos.second,ind1);
                   QTextStream(stdout) << "<<" << ind1 << "," << ind2+pos.second << "\n";
                }

                else if (seg2.coeff(ind1-1,ind2+pos.second))
                {
                    ind1 = ind1-1;
                    ind2 = ind2+pos.second;
                    pos.first = -1; pos.second = 0;
                    seg2.coeffRef(ind1,ind2) = 0;
                    seg2.prune(0,0);
                    curve << QPointF(ind2,ind1);
                    QTextStream(stdout) << "<<" << ind1 << "," << ind2 << "\n";
                    last1=false;
                }

                else if (seg2.coeff(ind1,ind2+pos.second))
                {
                    ind2 = ind2+pos.second;
                    pos.first = 1; pos.second = 1;
                    seg2.coeffRef(ind1,ind2) = 0;
                    seg2.prune(0,0);
                    curve << QPointF(ind2,ind1+1);
                    QTextStream(stdout) << "<<" << ind1+1 << "," << ind2 << "\n";
                    last1=false;
                }
                else
                {
                    break;
                }
            }
            else if(last1==false) //the last segment was from dimension 2
            {

                if (seg2.coeff(ind1+pos.first,ind2))
                {
                    ind1 = ind1+pos.first;
                    seg2.coeffRef(ind1,ind2) = 0;
                    seg2.prune(0,0);
                    curve << QPointF(ind2,ind1+pos.second);
                    QTextStream(stdout) << "<<" << ind1+pos.second << "," << ind2 << "\n";
                }

                else if (seg1.coeff(ind1+pos.second,ind2-1))
                {
                    ind1 = ind1+pos.second;
                    ind2 = ind2-1;
                    pos.first = -1; pos.second = 0;
                    seg1.coeffRef(ind1,ind2) = 0;
                    seg1.prune(0,0);
                    curve << QPointF(ind2,ind1);
                    QTextStream(stdout) << "<<" << ind1 << "," << ind2 << "\n";
                    last1 = true;
                }

                else if (seg1.coeff(ind1+pos.second,ind2))
                {
                    ind1 = ind1+pos.second;
                    pos.first = 1;  pos.second = 1;
                    seg1.coeffRef(ind1,ind2) = 0;
                    seg1.prune(0,0);
                    curve << QPointF(ind2+1,ind1);
                    QTextStream(stdout) << "<<" << ind1 << "," << ind2+1 << "\n";
                    last1 = true;
                }

                else
                {
                    break;
                }

            }
        }

//        for (int i=0; i<curve.size(); i++)
//        {
//            curve[i].setX(curve[i].rx()-1.5);
//            curve[i].setY(curve[i].ry()-1.5);
//        }

        if (curve.size()==1) //The fist segment couldn't be linked to some other segment; there is no polygon
            curve.clear();
        /****
        else
        {
            QTextStream(stdout) << "---curve---\n";
            for (int i=0; i<curve.size(); i++)
            {
                QTextStream(stdout) << curve[i].rx() << "," << curve[i].ry() << "\n";
            }
            QTextStream(stdout) << "------\n";
        }
        /**/
        return curve;

    }

    QVector<QVector<int>> findElement(const Eigen::MatrixXd &matrix, int value)
    {
        QVector<QVector<int>> coeffs(2,QVector<int>()); //i'm going to reverse this to match matlab ind1=row ind2=col
        for (int jx=0; jx<matrix.cols(); jx++) {
            for (int ix=0; ix<matrix.rows(); ix++) {
                if (matrix(ix,jx)==value)
                {
                    coeffs[0].push_back(ix);
                    coeffs[1].push_back(jx);
                }
            }
        }

        return coeffs;
    }


    QVector<QVector<int>> findNonZero(const Eigen::MatrixXd &matrix)
    {
        QVector<QVector<int>> coeffs(2,QVector<int>());

        for (int jx=0; jx<matrix.cols(); jx++) {
            for (int ix=0; ix<matrix.rows(); ix++) {
                if (matrix(ix,jx)!=0)
                {
                    coeffs[0].push_back(ix);
                    coeffs[1].push_back(jx);
                }
            }
        }

        return coeffs;
    }

    QVector<QVector<int>> findNonZero(const Eigen::SparseMatrix<int> &mat)
    {
        QVector<QVector<int>> coeffs(2,QVector<int>());

        for (int k=0; k<mat.outerSize(); ++k)
            for (Eigen::SparseMatrix<int>::InnerIterator it(mat,k); it; ++it)
            {
                coeffs[0].push_back(it.row());
                coeffs[1].push_back(it.col());
            }

        return coeffs;
    }
};





