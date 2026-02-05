#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "bufstring.h"
#include "uistring.h"

class uiColorInput;
class uiColorTableCanvas;
class uiColTabImport;
class uiColTabMarkerCanvas;
class uiFunctionDisplay;
class uiGenInput;
class uiLineEdit;
class uiPushButton;
class uiSpinBox;
class uiTable;
class uiTreeView;
class uiToolButton;
class uiWorld2Ui;

namespace ColTab { class Sequence; }

mExpClass(uiTools) uiTranspValuesDlg : public uiDialog
{ mODTextTranslationClass(uiColTabMarkerDlg);
public:
				uiTranspValuesDlg(uiParent*,ColTab::Sequence&,
						  const Interval<float>&);
				~uiTranspValuesDlg();

				Notifier<uiTranspValuesDlg> valuesChanged;
				Notifier<uiTranspValuesDlg> segmentInserted;
				Notifier<uiTranspValuesDlg> segmentRemoved;
				Notifier<uiTranspValuesDlg> markersChanged;


protected:

    uiTable*			table_;
    uiTable*			anchortable_	    = nullptr;
    uiPushButton*		syncanchors_;
    uiPushButton*		resettransp_;
    ColTab::Sequence&		ctab_;
    Interval<float>		ctabrange_;

    void			doFinalizeCB(CallBacker*);
    void			setPtsToAnchSegCB(CallBacker*);
    void			dataChgdCB(CallBacker*);
    void			rowInsertedCB(CallBacker*);
    void			rowDeletedCB(CallBacker*);
    void			resetTranspPointsCB(CallBacker*);
    void			mouseClickCB(CallBacker*);
    void			markerInsertedCB(CallBacker*);
    void			markerDeletedCB(CallBacker*);
    void			markerPositionChgdCB(CallBacker*);
    bool			acceptOK(CallBacker*);

    int				reverseAnchorIdx(int idx);
    int				reverseTransparencyIdx(int idx);
    int				reverseSegIdx(int idx);

    void			fillTableWithPoints();
    void			fillAnchorTable();
    void			fillTableWithSegments(bool resettransp);
    void			setPtsToAnchSeg(bool extrapolate);
    void			handleColorPos();
    void			handleDataVal();
    void			handleTranspVal();
};

namespace ColTab { class Sequence; }

mExpClass(uiTools) uiColorTableMan : public uiDialog
{ mODTextTranslationClass(uiColorTableMan);
public:
				uiColorTableMan(uiParent*,ColTab::Sequence&,
						bool enabletrans );
				~uiColorTableMan();

    const ColTab::Sequence&	currentColTab()	const	{ return ctab_; }

    void			setHistogram(const TypeSet<float>&);
    void			setHistogram(const TypeSet<float>&,
					     const Interval<float>&);
    const Interval<float>&	getRange() const;

    Notifier<uiColorTableMan> 	tableAddRem;
    Notifier<uiColorTableMan> 	tableChanged;
    Notifier<uiColorTableMan>	rangeChanged;

protected:

    uiFunctionDisplay*		cttranscanvas_;
    uiColorTableCanvas*		ctabcanvas_;
    uiColTabMarkerCanvas*	markercanvas_;
    uiTreeView*			coltablistfld_;
    uiToolButton*       	removebut_;
    uiToolButton*       	importbut_;
    uiToolButton*       	exportbut_;
    uiColorInput*       	undefcolfld_;
    uiColorInput*       	markercolfld_;
    uiGenInput*			segmentfld_;
    uiSpinBox*			nrsegbox_;
    uiLineEdit*			minfld_;
    uiLineEdit*			maxfld_;

    uiWorld2Ui*			w2uictabcanvas_;
    BufferString		selstatus_;
    ColTab::Sequence&         	ctab_;
    ColTab::Sequence*         	orgctab_		= nullptr;
    ColTab::Sequence*		segctab_		= nullptr;

    bool			issaved_		= true;
    int				selidx_			= -1;
    Interval<float>		ctabrange_;
    int				segidx_			= -1;

    bool			enabletrans_;

    void			doFinalizeCB(CallBacker*);
    void			reDrawCB(CallBacker*);
    void			markerChgdCB(CallBacker*);
    void			selChgdCB(CallBacker*);
    void			removeCB(CallBacker*);
    void			saveCB(CallBacker*);
    void			saveAsCB(CallBacker*);
    void			flipCB(CallBacker*);
    void			itemRenamedCB(CallBacker*);
    bool			acceptOK(CallBacker*) override;
    bool			rejectOK(CallBacker*) override;
    void			reDraw( bool deep ) override
				{ reDrawCB(nullptr); }


    void			refreshColTabList(const char*);
    void			updateTransparencyGraph();

    bool			saveColTab(bool);

    void			segmentSelCB(CallBacker*);
    void			insertSegmentCB(CallBacker*);
    void			removeSegmentCB(CallBacker*);
    void			nrSegmentsCB(CallBacker*);
    void			updateSegmentFields();

    void			undefColSelCB(CallBacker*);
    void			rangeChangedCB(CallBacker*);
    void			markerColChgdCB(CallBacker*);
    void			rightClickCB(CallBacker*);
    void			doSegmentize();
    void			importColTabCB(CallBacker*);
    void			exportColTabCB(CallBacker*);
    void			renameColTabCB(CallBacker*);
    void			transptSelCB(CallBacker*);
    void			transptChgCB(CallBacker*);
    void			sequenceChangeCB(CallBacker*);
    void			setPtsToAnchorSegmentsCB(CallBacker*);
    void			markerChangeCB(CallBacker*);
    void			markerDialogCB(CallBacker*);
    void			transpTableChgdCB(CallBacker*);
    void			rightClickTranspCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);

private:

    uiString			sKeyDefault();
    uiString			sKeyEdited();
    uiString			sKeyOwn();
};
