#ifndef uilayseqattribed_h
#define uilayseqattribed_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uilayseqattribed.h,v 1.4 2011-12-22 12:40:08 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiListBox;
class uiComboBox;
class uiGenInput;
class uiStratSelUnits;
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
    const Strat::RefTree& reftree_;
    bool		nmchgd_;
    bool		anychg_;

    uiGroup*		localgrp_;
    uiGroup*		slidegrp_;
    uiGenInput*		isslidingfld_;
    uiGenInput*		namefld_;
    uiGenInput*		valfld_;
    uiStratSelUnits*	unfld_;
    uiListBox*		lithofld_;
    uiComboBox*		stattypfld_;
    uiComboBox*		upscaletypfld_;
    uiComboBox*		transformfld_;

    const char*		gtDlgTitle(const Strat::LaySeqAttrib&,bool) const;
    void		putToScreen();
    bool		getFromScreen();

    void		initWin(CallBacker*);
    void		slSel(CallBacker*);
    void		transfSel(CallBacker*);

    bool		acceptOK(CallBacker*);

};

#endif
