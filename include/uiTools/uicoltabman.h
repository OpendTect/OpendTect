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
class uiColTabMarkerCanvas;
class uiFunctionDisplay;
class uiGenInput;
class uiTreeView;
class uiToolButton;
class uiSpinBox;
class uiWorld2Ui;
class uiColTabImport;
class uiTable;

namespace ColTab { class Sequence; }

mExpClass(uiTools) uiTranspValuesDlg : public uiDialog
{ mODTextTranslationClass(uiColTabMarkerDlg);
public:
				uiTranspValuesDlg(uiParent*,ColTab::Sequence&,
						  const Interval<float>&);
				~uiTranspValuesDlg();

    Notifier<uiTranspValuesDlg> valuesChanged;


protected:

    uiTable*			table_;
    ColTab::Sequence&		ctab_;
    Interval<float>		ctabrange_;

    void			dataChgdCB(CallBacker*);
    void			pointInsertedCB(CallBacker*);
    void			pointDeletedCB(CallBacker*);

    void			fillTable();
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

    Notifier<uiColorTableMan> 	tableAddRem;
    Notifier<uiColorTableMan> 	tableChanged;

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
    uiWorld2Ui*			w2uictabcanvas_;

    BufferString		selstatus_;
    ColTab::Sequence&         	ctab_;
    ColTab::Sequence*         	orgctab_		= nullptr;

    bool			issaved_		= true;
    int				selidx_			= -1;
    Interval<float>		ctabrange_;

    bool			enabletrans_;

    void			doFinalize(CallBacker*);
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
    void			nrSegmentsCB(CallBacker*);
    void			updateSegmentFields();

    void			undefColSelCB(CallBacker*);
    void			markerColChgdCB(CallBacker*);
    void			rightClickColorCB(CallBacker*);
    void			rightClickTranspCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);
    void			doSegmentize();
    void			importColTabCB(CallBacker*);
    void			exportColTabCB(CallBacker*);
    void			renameColTabCB(CallBacker*);
    void			transptSelCB(CallBacker*);
    void			transptChgdCB(CallBacker*);
    void			sequenceChangeCB(CallBacker*);
    void			markerChangeCB(CallBacker*);
    void			transpTableChgd(CallBacker*);

private:

    uiString			sKeyDefault();
    uiString			sKeyEdited();
    uiString			sKeyOwn();
};
