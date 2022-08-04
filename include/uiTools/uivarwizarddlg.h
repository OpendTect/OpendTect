#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uistring.h"


/*!\brief Dialog in flexible wizard series. */

mExpClass(uiTools) uiVarWizardDlg : public uiDialog
{ mODTextTranslationClass(uiVarWizardDlg);
public:

    enum Position	{ Start, Middle, End, DoWork };
			// End and DoWork are the same (end-)position, but
			// one says 'Finish', the other 'Go'.

			uiVarWizardDlg(uiParent*,const uiDialog::Setup&,IOPar&,
					Position,bool revbuttons=true);
			~uiVarWizardDlg();

protected:

    IOPar&		pars_;
    Position		pos_;

    bool		leave_;
    bool		rejectOK(CallBacker*) override;

    friend class	uiVarWizard;

};


