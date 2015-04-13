#ifndef BSTREE_H
#define BSTREE_H

#include <QList>
#include <QLineF>
#include "linesegment.h"

class lineSegment;
class BSTree
{
    class Node
    {
        friend class BSTree;
        lineSegment *ls;
        Node *parent;
        Node *left;
        Node *right;

    public:
        Node(lineSegment);
        //Node(qreal, qreal, qreal, qreal);
        bool isRoot() { return (parent == NULL); }
        bool hasLeft() { return (left!=NULL); }
        bool hasRight() { return (right!=NULL); }
        bool amAbove(lineSegment);
        bool isEqual(lineSegment);
    };

    int size;    //???
    BSTree::Node * root;
    QList<QLineF> lines;

public:
    BSTree();
    ~BSTree();


    bool empty() { return (size==0); }
    BSTree::Node* findNode(lineSegment,Node*);
    void insert(lineSegment,Node*);
    void swap(lineSegment,lineSegment);
    bool remove(lineSegment,Node*);
    BSTree::Node* findLeft(Node*);


};

#endif // BSTREE_H
