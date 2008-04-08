#ifndef uicoltabman_h
#define uicoltabman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          February 2008
 RCS:           $Id: uicoltabman.h,v 1.1 2008-04-08 04:56:10 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "coltab.h"
#include "coltabsequence.h"
#include "bufstringset.h"

class uiColorTableCanvas;
class uiFunctionDisplay;
class IOPar;
class uiCanvas;
class uiCheckBox;
class uiColorInput;
class uiGenInput;
class uiListView;
class uiPushButton;
class uiSpinBox;
class uiWorld2Ui;


class uiColorTableMan : public uiDialog
{
public:
				uiColorTableMan(uiParent*,ColTab::Sequence&);
				~uiColorTableMan();

    const ColTab::Sequence&	currentColTab()	const	{ return ctab_; }

    void			setHistogram(const TypeSet<float>&);

    Notifier<uiColorTableMan> 	tableadded;
    Notifier<uiColorTableMan> 	applycb;
    Notifier<uiColorTableMan> 	markerchanged;
    Notifier<uiColorTableMan> 	selectionchanged;

protected:

    uiFunctionDisplay*		cttranscanvas_;
    uiColorTableCanvas*		ctabcanvas_;
    uiCanvas*			markercanvas_;
    uiListView*			coltablistfld_;
    uiPushButton*       	removebut_;
    uiColorInput*       	undefcolfld_;
    uiCheckBox*			segmentfld_;
    uiSpinBox*			nrsegbox_;
    uiWorld2Ui*			w2uimarker_;
    uiWorld2Ui*			w2uictabcanvas_;

    BufferString		selstatus_;
    ColTab::Sequence&         	ctab_;
    ColTab::Sequence*         	orgctab_;

    NamedBufferStringSet	editedctnms_;
    NamedBufferStringSet	sysctnms_;
    NamedBufferStringSet	usrctnms_;
    NamedBufferStringSet	allctnms_;
    BufferStringSet		status_;

    bool			issaved_;
    int				selidx;

    void			doFinalise(CallBacker*);
    void			selChg(CallBacker*);
    void			removeCB(CallBacker*);
    void			saveCB(CallBacker*);
    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);

    void			mouseClk(CallBacker*);
    void			mouse2Clk(CallBacker*);
    void			mouseRelease(CallBacker*);
    void			mrkrmouseMove(CallBacker*);

    void			reFillTrans(CallBacker*);

    void			refreshColTabList(const char*);

    void			addMarker(float,bool);
    void			removeMarker(int);
    void			changeColor(int);
    bool			saveColTab(bool);
    void			readFromSettings(ObjectSet<IOPar>&);
    void			writeToSettings(const ObjectSet<IOPar>&);

//    void			transparencyChange(CallBacker*);
    void			doApply(CallBacker*);
//    void			mouseMove(CallBacker*);
    void			segmentSel(CallBacker*);
    void			nrSegmentsCB(CallBacker*);
    void			rightClick(CallBacker*);
    void			doSegmentize();
    void			drawMarkers(CallBacker*);
};

#endif
