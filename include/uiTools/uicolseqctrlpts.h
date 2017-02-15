#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          April 2008
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uigraphicsview.h"
#include "uistring.h"

namespace ColTab{ class Sequence; }
class MouseEvent;
class MouseEventHandler;
class uiGraphicsItemGroup;
class uiTable;
class uiParent;


mExpClass(uiTools) uiColSeqColCtrlPtsDlg : public uiDialog
{ mODTextTranslationClass(uiColSeqColCtrlPtsDlg);
public:

    typedef ColTab::Sequence	Sequence;

			uiColSeqColCtrlPtsDlg(uiParent*,Sequence&);

protected:

    uiTable*		table_;
    Sequence&		colseq_;
    ConstRefMan<Sequence> rollbackcseq_;

    void		fillTable();
    void		mouseClick(CallBacker*);
    void		pointInserted(CallBacker*);
    void		pointDeleted(CallBacker*);
    void		pointPosChgd(CallBacker*);
    void		seqChgCB(CallBacker*);
    bool		rejectOK();

};


mExpClass(uiTools) uiColSeqColCtrlPtsDisp : public uiGraphicsView
{ mODTextTranslationClass(uiColSeqColCtrlPtsDisp);
public:

    typedef ColTab::Sequence	Sequence;

				uiColSeqColCtrlPtsDisp(uiParent*);

    void			setSequence(Sequence&);

protected:

    uiParent*		parent_;
    uiGraphicsItemGroup* markerlineitmgrp_;
    RefMan<Sequence>	colseq_;
    MouseEventHandler&	meh_;
    int			curcptidx_;

    void		drawMarkers(CallBacker*);
    void		mousePress(CallBacker*);
    void		mouseMove(CallBacker*);
    void		mouseRelease(CallBacker*);
    void		mouseDoubleClk(CallBacker*);
    void		seqChgCB(CallBacker*);

    void		addMarker(float,bool);
    void		removeMarker(int);
    bool		changeColor(int);

};
