#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uigraphicsview.h"
#include "uistring.h"

namespace ColTab{ class Sequence; }
class MouseEventHandler;
class uiGraphicsItemGroup;
class uiTable;
class uiParent;
class uiWorld2Ui;


mExpClass(uiTools) uiColTabMarkerDlg : public uiDialog
{ mODTextTranslationClass(uiColTabMarkerDlg);
public:
				uiColTabMarkerDlg(uiParent*,ColTab::Sequence&);
				~uiColTabMarkerDlg();

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
    bool			acceptOK(CallBacker*) override;

};


mExpClass(uiTools) uiColTabMarkerCanvas : public uiGraphicsView
{ mODTextTranslationClass(uiColTabMarkerCanvas);
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
