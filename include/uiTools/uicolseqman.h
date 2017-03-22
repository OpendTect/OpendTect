#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          February 2008
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "coltabsequence.h"
#include "datadistribution.h"

class uiColorInput;
class uiColSeqDisp;
class uiColSeqColCtrlPtsEd;
class uiFunctionDisplay;
class uiGenInput;
class uiTreeView;
class uiButton;
class uiLabeledSpinBox;
class uiColTabImport;


mExpClass(uiTools) uiColSeqMan : public uiDialog
{ mODTextTranslationClass(uiColSeqMan);
public:

    typedef ColTab::Sequence		Sequence;

			uiColSeqMan(uiParent*,const char* initialseqnm=0);
			~uiColSeqMan();
			mDeclInstanceCreatedNotifierAccess(uiColSeqMan);

    const Sequence&	current() const		    { return *curseq_; }

    Notifier<uiColSeqMan> selectionChanged;

protected:

    uiTreeView*			seqlistfld_;
    uiFunctionDisplay*		transpdisp_;
    uiColSeqColCtrlPtsEd*	ctrlptsed_;
    uiColSeqDisp*		seqdisp_;
    uiColorInput*		undefcolfld_;
    uiColorInput*		markcolfld_;
    uiGenInput*			segtypefld_;
    uiLabeledSpinBox*		nrsegfld_;
    uiButton*			removebut_;

    RefMan<ColTab::Sequence>	curseq_;
    RefMan<ColTab::Sequence>	rollbackseq_;
    ColTab::SequenceManager&	seqmgr_;
    ColTab::SequenceManager*	rollbackmgr_;
    bool			mgrsaved_;

    friend class		uiColSeqColCtrlPtsEd;

    bool			save();
    bool			acceptOK();
    bool			rejectOK();

    void			setSegmentation();
    void			handleSeqChg();

    void			updateColSeqList();
    void			updateTransparencyGraph();
    void			updateSegmentationFields();
    void			updateSpecColFlds();
    void			updateActionStates();

    void			doFinalise(CallBacker*);
    void			selChgCB(CallBacker*);
    void			transpPtSelCB(CallBacker*);
    void			transpPtChgCB(CallBacker*);
    void			segmentTypeSelCB(CallBacker*);
    void			nrSegmentsChgCB(CallBacker*);
    void			undefColSelCB(CallBacker*);
    void			markerColSelCB(CallBacker*);
    void			impColSeqCB(CallBacker*);
    void			removeCB(CallBacker*);
    void			toggleDisabledCB(CallBacker*);
    void			seqChgCB(CallBacker*);
    void			seqMgrChgCB(CallBacker*);

};
