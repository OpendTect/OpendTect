#ifndef flatviewaxesdrawer_h
#define flatviewaxesdrawer_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "drawaxis2d.h"

class uiGraphicsView;
class uiGraphicsScene;

namespace FlatView
{
class Viewer;

/*!
\brief Axis drawer for flat viewers.
*/

mExpClass(uiFlatView) AxesDrawer : public ::uiGraphicsSceneAxisMgr
{
public:
    			AxesDrawer(Viewer&,uiGraphicsView&);

    int			altdim0_;
    void		update();

protected:

    Viewer&		vwr_;
    virtual double      getAnnotTextAndPos(bool,double,BufferString*) const;

};

} // namespace

#endif

