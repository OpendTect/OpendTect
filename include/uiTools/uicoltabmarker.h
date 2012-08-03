#ifndef uicoltabmarker_h
#define uicoltabmarker_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          April 2008
 RCS:           $Id: uicoltabmarker.h,v 1.7 2012-08-03 13:01:12 cvskris Exp $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uigraphicsview.h"

namespace ColTab{ class Sequence; }
class MouseEventHandler;
class uiGraphicsItemGroup;
class uiTable;
class uiParent;
class uiWorld2Ui;


mClass(uiTools) uiColTabMarkerDlg : public uiDialog
{
public:
				uiColTabMarkerDlg(uiParent*,ColTab::Sequence&);

    Notifier<uiColTabMarkerDlg>	markersChanged;

protected:

    uiTable*			table_;
    ColTab::Sequence&		ctab_;

    void			fillTable();
    void			rebuildColTab();
    void			mouseClick(CallBacker*);
    void			markerInserted(CallBacker*);
    void			markerDeleted(CallBacker*);
    void			markerPosChgd(CallBacker*);
    bool			acceptOK(CallBacker*);

};


mClass(uiTools) uiColTabMarkerCanvas : public uiGraphicsView
{
public:
				uiColTabMarkerCanvas(uiParent*,
						     ColTab::Sequence&);
				~uiColTabMarkerCanvas();

    Notifier<uiColTabMarkerCanvas> markerChanged;

protected:

    uiWorld2Ui*                 w2ui_;
    uiParent*	                parent_;
    uiGraphicsItemGroup*	markerlineitmgrp_;
    int		                selidx_;
    ColTab::Sequence&           ctab_;
    MouseEventHandler&		meh_;

    void                        addMarker(float,bool);
    void                        removeMarker(int);
    bool                        changeColor(int);

    void                        drawMarkers(CallBacker*);
    void                        mouseClk(CallBacker*);
    void                        mouse2Clk(CallBacker*);
    void                        mouseRelease(CallBacker*);
    void                        mouseMove(CallBacker*);
    void                        markerChgd(CallBacker*);
};

#endif

