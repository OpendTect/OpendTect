#ifndef uimultiflatviewcontrol_h
#define uimultiflatviewcontrol_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2012
 RCS:           $Id: uimultiflatviewcontrol.h,v 1.2 2012-02-16 15:40:43 cvsbruno Exp $
________________________________________________________________________

-*/

/*! brief : a control for FlatViewers with different zoom properties !*/

#include "uiflatviewstdcontrol.h"

mClass uiMultiFlatViewControl : public uiFlatViewStdControl
{
public:
    			uiMultiFlatViewControl(uiFlatViewer&,const Setup&);
    			~uiMultiFlatViewControl();

    void                setNewView(Geom::Point2D<double>& centre,
	                                       Geom::Size2D<double>& sizes);

    uiToolBar*		getToolBar(int idx) { return toolbars_[idx]; }

protected:
    ObjectSet<FlatView::ZoomMgr> zoommgrs_;
    ObjectSet<uiToolBar> toolbars_;

    bool		handleUserClick();
    void		reInitZooms();

    void		editCB( CallBacker* );
    void		rubBandCB(CallBacker*);
    void		dataChangeCB(CallBacker*);
    void		vwrAdded(CallBacker*);
    void		zoomCB(CallBacker*);
};

#endif
