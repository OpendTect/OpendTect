#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "stratsynthdatamgr.h"
#include "uidialog.h"

class uiButton;
class uiListBox;
class uiSynthGenParams;
namespace SynthSeis { class GenParams; }


mExpClass(uiWellAttrib) uiStratSynthDataMgr : public uiDialog
{ mODTextTranslationClass(uiStratSynthDataMgr);
public:

    typedef StratSynth::DataMgr		DataMgr;
    typedef DataMgr::SynthID		SynthID;
    typedef SynthSeis::GenParams	GenParams;
    typedef SynthSeis::SyntheticType	SynthType;

			uiStratSynthDataMgr(uiParent*,DataMgr&);
			~uiStratSynthDataMgr();

    SynthID		curID() const;
    const GenParams&	curGenPars() const;

    Notifier<uiStratSynthDataMgr> applyReq;
    Notifier<uiStratSynthDataMgr> selChg;

protected:

    DataMgr&			mgr_;
    int				prevselidx_	= 1;

    uiListBox*			selfld_;
    uiSynthGenParams*		gpfld_;
    uiButton*			rmbut_;
    uiButton*			addasnewbut_;

    void			updAddNewBut();
    void			updRmBut();
    void			updUi();
    void			commitEntry(int);
    SynthID			idForIdx(int) const;

    void			initWin(CallBacker*);
    void			addAsNewCB(CallBacker*);
    void			rmCB(CallBacker*);
    void			selChgCB(CallBacker*);
    void			nmChgCB(CallBacker*);

    bool			applyOK() override;
    bool			rejectOK() override;

};
