#ifndef uicoltabmarker_h
#define uicoltabmarker_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          April 2008
 RCS:           $Id: uicoltabmarker.h,v 1.2 2008-07-16 09:30:39 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uicanvas.h"

namespace ColTab{ class Sequence; }
class uiTable;
class uiParent;
class uiWorld2Ui;


class uiColTabMarkerDlg : public uiDialog
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


class uiColTabMarkerCanvas : public uiCanvas
{
public:
				uiColTabMarkerCanvas(uiParent*,
						     ColTab::Sequence&);
				~uiColTabMarkerCanvas();

    bool                       	isSegmentized();
    Notifier<uiColTabMarkerCanvas> markerChanged;

protected:

    uiWorld2Ui*                 w2ui_;
    uiParent*	                parent_;
    int		                selidx_;
    ColTab::Sequence&           ctab_;

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
