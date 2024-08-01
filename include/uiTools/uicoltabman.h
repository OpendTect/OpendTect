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


namespace ColTab { class Sequence; }

mExpClass(uiTools) uiColorTableMan : public uiDialog
{ mODTextTranslationClass(uiColorTableMan);
public:
				uiColorTableMan(uiParent*,ColTab::Sequence&,
						bool enabletrans );
				~uiColorTableMan();

    const ColTab::Sequence&	currentColTab()	const	{ return ctab_; }

    void			setHistogram(const TypeSet<float>&);

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

    bool			enabletrans_;

    void			doFinalize(CallBacker*);
    void			reDrawCB(CallBacker*);
    void			markerChgd(CallBacker*);
    void			selChg(CallBacker*);
    void			removeCB(CallBacker*);
    void			saveCB(CallBacker*);
    void			saveAsCB(CallBacker*);
    void			flipCB(CallBacker*);
    void			itemRenamedCB(CallBacker*);
    bool			acceptOK(CallBacker*) override;
    bool			rejectOK(CallBacker*) override;
    void			reDraw( bool deep ) override	{ reDrawCB(0); }


    void			refreshColTabList(const char*);
    void			updateTransparencyGraph();

    bool			saveColTab(bool);

    void			segmentSel(CallBacker*);
    void			nrSegmentsCB(CallBacker*);
    void			updateSegmentFields();

    void			undefColSel(CallBacker*);
    void			markerColChgd(CallBacker*);
    void			rightClick(CallBacker*);
    void			doSegmentize();
    void			importColTab(CallBacker*);
    void			exportColTab(CallBacker*);
    void			renameColTab(CallBacker*);
    void			transptSel(CallBacker*);
    void			transptChg(CallBacker*);
    void			sequenceChange(CallBacker*);
    void			markerChange(CallBacker*);

private:

    uiString			sKeyDefault();
    uiString			sKeyEdited();
    uiString			sKeyOwn();
};
