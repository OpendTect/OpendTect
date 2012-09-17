#ifndef uilayseqattribed_h
#define uilayseqattribed_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uilayseqattribed.h,v 1.5 2012/04/10 14:29:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiListBox;
class uiComboBox;
class uiGenInput;
class uiStratSelUnits;
namespace Strat { class LaySeqAttrib; class RefTree; };


/*! \brief edits a layer sequence attribute */

mClass uiLaySeqAttribEd : public uiDialog
{
public:

    mClass Setup
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

    bool		acceptOK(CallBacker*);

};

#endif
