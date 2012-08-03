#ifndef flatviewaxesdrawer_h
#define flatviewaxesdrawer_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: flatviewaxesdrawer.h,v 1.9 2012-08-03 13:00:57 cvskris Exp $
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "drawaxis2d.h"

class uiGraphicsView;
class uiGraphicsScene;
namespace FlatView
{

class Viewer;

/*!\brief Axis drawer for flat viewers */

mClass(uiFlatView) AxesDrawer : public ::uiGraphicsSceneAxisMgr
{
public:
    			AxesDrawer(Viewer&,uiGraphicsView&);
    

    int			altdim0_;

protected:

    Viewer&		vwr_;

    virtual double      getAnnotTextAndPos(bool,double,BufferString*) const;

};

} // namespace

#endif

