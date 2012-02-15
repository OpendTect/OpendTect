#ifndef uimultiflatviewcontrol_h
#define uimultiflatviewcontrol_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2012
 RCS:           $Id: uimultiflatviewcontrol.h,v 1.1 2012-02-15 15:52:29 cvsbruno Exp $
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

protected:
    ObjectSet<FlatView::ZoomMgr> zoommgrs_;

    void		editCB( CallBacker* );

    void		rubBandCB(CallBacker*);
    void		dataChangeCB(CallBacker*);
    void		reInitZooms();

    void		vwrAdded(CallBacker*);
    void		zoomCB(CallBacker*);
};

#endif
