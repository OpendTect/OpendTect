#ifndef flatviewaxesdrawer_h
#define flatviewaxesdrawer_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: flatviewaxesdrawer.h,v 1.6 2009/07/22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "drawaxis2d.h"

class uiGraphicsView;
class uiGraphicsScene;
namespace FlatView
{

class Viewer;

/*!\brief Axis drawer for flat viewers */

mClass AxesDrawer : public ::DrawAxis2D
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
