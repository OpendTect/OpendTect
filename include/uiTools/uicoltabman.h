#ifndef uicoltabman_h
#define uicoltabman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          February 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "bufstring.h"

class uiColorInput;
class uiColorTableCanvas;
class uiColTabMarkerCanvas;
class uiFunctionDisplay;
class uiGenInput;
class uiTreeView;
class uiPushButton;
class uiSpinBox;
class uiWorld2Ui;

namespace ColTab { class Sequence; }

mExpClass(uiTools) uiColorTableMan : public uiDialog
{
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
    uiPushButton*       	removebut_;
    uiPushButton*       	importbut_;
    uiColorInput*       	undefcolfld_;
    uiColorInput*       	markercolfld_;
    uiGenInput*			segmentfld_;
    uiSpinBox*			nrsegbox_;
    uiWorld2Ui*			w2uictabcanvas_;

    BufferString		selstatus_;
    ColTab::Sequence&         	ctab_;
    ColTab::Sequence*         	orgctab_;

    bool			issaved_;
    int				selidx_;

    bool			enabletrans_;

    void			doFinalise(CallBacker*);
    void			reDrawCB(CallBacker*);
    void			markerChgd(CallBacker*);
    void			selChg(CallBacker*);
    void			removeCB(CallBacker*);
    void			saveCB(CallBacker*);
    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
    virtual void		reDraw( bool deep )	{ reDrawCB(0); }


    void			refreshColTabList(const char*);

    bool			saveColTab(bool);

    void			segmentSel(CallBacker*);
    void			nrSegmentsCB(CallBacker*);
    void			updateSegmentFields();

    void			undefColSel(CallBacker*);
    void			markerColChgd(CallBacker*);
    void			rightClick(CallBacker*);
    void			doSegmentize();
    void			importColTab(CallBacker*);
    void			transptSel(CallBacker*);
    void			transptChg(CallBacker*);
    void			sequenceChange(CallBacker*);
    void			markerChange(CallBacker*);
};

#endif

