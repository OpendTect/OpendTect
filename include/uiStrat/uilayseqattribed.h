#ifndef uilayseqattribed_h
#define uilayseqattribed_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uilayseqattribed.h,v 1.3 2011-01-25 09:41:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiListBox;
class uiComboBox;
class uiGenInput;
namespace Strat { class LaySeqAttrib; class RefTree; };


/*! \brief Dialog for creating volume output */

mClass uiLaySeqAttribEd : public uiDialog
{
public:

			uiLaySeqAttribEd(uiParent*,Strat::LaySeqAttrib&,
					 const Strat::RefTree&,bool isnew);
			~uiLaySeqAttribEd();

    bool		anyChange() const	{ return anychg_; }
    bool		nameChanged() const	{ return nmchgd_; }

protected:

    Strat::LaySeqAttrib& attr_;
    bool		nmchgd_;
    bool		anychg_;

    uiGroup*		localgrp_;
    uiGroup*		slidegrp_;
    uiGenInput*		isslidingfld_;
    uiGenInput*		namefld_;
    uiGenInput*		valfld_;
    uiListBox*		unfld_;
    uiListBox*		lithofld_;
    uiComboBox*		stattypfld_;
    uiComboBox*		upscaletypfld_;
    uiComboBox*		transformfld_;

    void		fillFlds(const Strat::RefTree&);
    const char*		gtDlgTitle(const Strat::LaySeqAttrib&,bool) const;
    void		putToScreen();
    bool		getFromScreen();

    void		initWin(CallBacker*);
    void		slSel(CallBacker*);
    void		transfSel(CallBacker*);

    bool		acceptOK(CallBacker*);

};

#endif
