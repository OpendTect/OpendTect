#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uidialog.h"
#include "uistring.h"

class uiListBox;
class uiComboBox;
class uiGenInput;
class uiStratSelUnits;
namespace Strat { class LaySeqAttrib; class RefTree; };


/*! \brief edits a layer sequence attribute */

mExpClass(uiStrat) uiLaySeqAttribEd : public uiDialog
{ mODTextTranslationClass(uiLaySeqAttribEd);
public:

    mExpClass(uiStrat) Setup
    {
    public:
			Setup( bool isnw )
			    : isnew_(isnw)
			    , allowlocal_(true)
			    , allowintegr_(true)	{}

	mDefSetupMemb(bool,isnew)
	mDefSetupMemb(bool,allowlocal)
	mDefSetupMemb(bool,allowintegr)
    };

			uiLaySeqAttribEd(uiParent*,Strat::LaySeqAttrib&,
					 const Strat::RefTree&,const Setup&);
			~uiLaySeqAttribEd();

    bool		anyChange() const	{ return anychg_; }
    bool		nameChanged() const	{ return nmchgd_; }

protected:

    Strat::LaySeqAttrib& attr_;
    const Strat::RefTree& reftree_;
    bool		nmchgd_;
    bool		anychg_;

    uiGroup*		localgrp_;
    uiGroup*		integrgrp_;
    uiGenInput*		islocalfld_;
    uiGenInput*		namefld_;
    uiGenInput*		valfld_;
    uiStratSelUnits*	unfld_;
    uiListBox*		lithofld_;
    uiComboBox*		stattypfld_;
    uiComboBox*		upscaletypfld_;
    uiComboBox*		transformfld_;

    inline bool		haveLocal() const	{ return localgrp_; }
    inline bool		haveIntegrated() const	{ return integrgrp_; }
    bool		isLocal() const;
    void		putToScreen();
    bool		getFromScreen();

    void		initWin(CallBacker*);
    void		slSel(CallBacker*);
    void		transfSel(CallBacker*);

    bool		acceptOK(CallBacker*) override;

};
