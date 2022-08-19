#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
