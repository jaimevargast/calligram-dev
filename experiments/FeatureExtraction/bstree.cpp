#include "bstree.h"
#include "linesegment.h"
#include <QPointF>


BSTree::BSTree()
{
    root = NULL;
    size=0;
    lines.clear();
}

BSTree::~BSTree()
{

}
//********** Line Segment Class *****************
BSTree::Node::Node(lineSegment l)
{
    BSTree::Node::ls=&l;
    BSTree::Node::parent = NULL;
    BSTree::Node::left = NULL;
    BSTree::Node::right = NULL;
}

bool BSTree::Node::amAbove(lineSegment l2)
{

    //Three points are a counter-clockwise turn if ccw > 0, clockwise if
    //ccw < 0, and collinear if ccw = 0 because ccw is a determinant that
    //gives twice the signed  area of the triangle formed by p1, p2 and p3.
    //function ccw(p1, p2, p3):
    //    return (p2.x - p1.x)*(p3.y - p1.y) - (p2.y - p1.y)*(p3.x - p1.x)

    QPointF p1 = ls->start();
    QPointF p2 = ls->end();
    QPointF p3 = l2.start();


    if ((p2.rx()-p1.rx())*(p3.ry()-p1.ry())-(p2.ry()-p1.ry())*(p3.rx()-p1.rx())>0) // l2 is above me
        return 0;
    else        // I'm above l2
        return 1;
}

bool BSTree::Node::isEqual(lineSegment l2)
{
    return ((ls->start()==l2.start())&&(ls->end()==l2.end()));
}
//************************************************************


BSTree::Node* BSTree::findNode(lineSegment l, Node * n)
{
//    function Find-recursive(key, node):  // call initially with node = root
//        if node = Null or node.key = key then
//            return node
//        else if key < node.key then
//            return Find-recursive(key, node.left)
//        else
//            return Find-recursive(key, node.right)
    if ((n==NULL)||n->isEqual(l))
    {
        return n;
    }
    else if (n->amAbove(l))
    {
        return findNode(l,n->left);
    }
    else
    {
        return findNode(l,n->right);
    }
}

void BSTree::insert(lineSegment l, Node * n)
{
    if (root==NULL)     // Tree is empty
    {
        root = new Node(l);
    }
    else if (n->amAbove(l))
    {
        if (n->hasLeft())   // Recurse
        {
            insert(l,n->left);
        }
        else                // Node is a Leaf; insert node here
        {
            n->left = new Node(l);
            n->left->parent = n;
        }
    }
    else // not above
    {
        if (n->hasRight()) // Recurse
        {
            insert(l,n->right);
        }
        else                // Node is a Leaf; insert node here
        {
            n->right = new Node(l);
            n->right->parent = n;
        }
    }
}

void BSTree::swap(lineSegment l1, lineSegment l2)
{
    BSTree::Node * n1 = findNode(l1,root);
    BSTree::Node * n2 = findNode(l2,root);
    if (n1 != NULL && n2 != NULL)
    {
        lineSegment * temp = n1->ls;
        n1->ls = n2->ls;
        n2->ls = temp;
    }
}

bool BSTree::remove(lineSegment l, Node * n)
{
    Node * temp = findNode(l,n);

    if (!temp==NULL)
    {
        // Case 1: has no children
        if (!temp->hasLeft()&&!temp->hasRight())
        {
            if(temp->parent->left==temp)
                temp->parent->left=NULL;
            else
                temp->parent->right=NULL;
            delete temp;
            return true;
        }
        // Case 2: has only one child
        else if (temp->hasLeft()&&!temp->hasRight())    // has left but no right
        {
            temp->parent->left = temp->left;
            delete temp;
            return true;
        }
        else if (!temp->hasLeft()&&temp->hasRight())    // has right but no left
        {
            temp->parent->right = temp->right;
            delete temp;
            return true;
        }

        // Case 3: has both left and right
        else if (temp->hasLeft()&&temp->hasRight())
        {
            BSTree::Node * min = findLeft(temp->right);
            temp->ls = min->ls;
            min->parent->left = NULL;
            delete min;
            return true;
        }
    }
    else
    {
        return false;
    }
}

BSTree::Node* BSTree::findLeft(Node *n)
{
    BSTree::Node * temp = n;
    while (temp->hasLeft())
    {
        temp = temp->left;
    }
    return temp;
}

