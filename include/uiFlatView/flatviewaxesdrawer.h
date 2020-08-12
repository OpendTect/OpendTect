#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicssceneaxismgr.h"

class uiFlatViewer;
class uiScaleBarItem;

/*!
\brief Axis drawer for flat viewers.
*/

mExpClass(uiFlatView) AxesDrawer : public ::uiGraphicsSceneAxisMgr
{
public:
			AxesDrawer(uiFlatViewer&);
			~AxesDrawer();

    int			altdim0_	= -1;
    void		updateScene();
    void		setZValue(int z);
    void		updateViewRect();
    uiRect		getViewRect(bool withextraborders=true) const;
    void		setWorldCoords(const uiWorldRect&);
    void		setExtraBorder(const uiBorder&);
    uiBorder		getAnnotBorder(bool withextraborders=true) const;

protected:

    uiFlatViewer&	vwr_;
    uiRectItem*		rectitem_;
    uiTextItem*		axis1nm_;
    uiTextItem*		axis2nm_;
    uiTextItem*		titletxt_;
    uiArrowItem*	arrowitem1_;
    uiArrowItem*	arrowitem2_;
    uiScaleBarItem*	scalebaritem_;
    uiBorder		extraborder_;

    void		transformAndSetAuxAnnotation(bool forx1);
    void		setScaleBarWorld2UI(const uiWorldRect&);
};

