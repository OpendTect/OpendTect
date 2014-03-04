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
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicssceneaxismgr.h"

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
			~AxesDrawer();

    int			altdim0_;
    void		updateScene();
    void		setZValue(int z);
    void		updateViewRect();
    uiRect		getViewRect() const;
    void		setWorldCoords(const uiWorldRect&);
    void		setExtraBorder(const uiBorder&);
    uiBorder		getAnnotBorder() const;

protected:

    Viewer&		vwr_;
    uiRectItem*         rectitem_;
    uiTextItem*         axis1nm_;
    uiTextItem*         axis2nm_;
    uiTextItem*		titletxt_;
    uiArrowItem*        arrowitem1_;
    uiArrowItem*        arrowitem2_;
    uiBorder		extraborder_;

};

} // namespace

#endif

