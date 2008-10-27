#ifndef flatviewaxesdrawer_h
#define flatviewaxesdrawer_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: flatviewaxesdrawer.h,v 1.4 2008-10-27 11:21:08 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "drawaxis2d.h"

class uiGraphicsView;
class uiGraphicsScene;
namespace FlatView
{

class Viewer;

/*!\brief Axis drawer for flat viewers */

class AxesDrawer : public ::DrawAxis2D
{
public:
    			AxesDrawer(Viewer&,uiGraphicsView&);

    void		draw(uiRect,uiWorldRect);

    int			altdim0_;

protected:

    Viewer&		vwr_;

    virtual double      getAnnotTextAndPos(bool,double,BufferString*) const;

};

} // namespace

#endif
