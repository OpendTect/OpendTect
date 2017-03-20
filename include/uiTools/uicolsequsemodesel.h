#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2017
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "coltab.h"
#include "uigroup.h"

class uiToolBar;
class uiCheckBox;
class uiColSeqUseModeCompactSel;


/* uiToolBar color sequence use mode selection */

mExpClass(uiTools) uiColSeqUseModeSel : public uiGroup
{ mODTextTranslationClass(uiColSeqUseModeSel);
public:

			uiColSeqUseModeSel(uiParent*,bool compact,
					   uiString lbltxt=tr("Use table"));
			~uiColSeqUseModeSel();

    ColTab::SeqUseMode	mode() const;
    void		setMode(ColTab::SeqUseMode);
    void		addObjectsToToolBar(uiToolBar&);

    Notifier<uiColSeqUseModeSel>    modeChange;

protected:

    uiCheckBox*		flippedbox_;
    uiCheckBox*		cyclicbox_;
    uiColSeqUseModeCompactSel* compactsel_;

    void		modeChgCB( CallBacker* )	{ modeChange.trigger();}
    void		canvasMouseReleaseCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);

};
