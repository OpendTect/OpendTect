#ifndef uivarwizarddlg_h
#define uivarwizarddlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"


/*!\brief Dialog in flexible wizard series. */

mExpClass(uiTools) uiVarWizardDlg : public uiDialog
{
public:

    enum Position	{ Start, Middle, End, DoWork };
			// End and DoWork are the same (end-)position, but
			// one says 'Finish', the other 'Go'.

			uiVarWizardDlg(uiParent*,const uiDialog::Setup&,IOPar&,
					Position);
			~uiVarWizardDlg();

protected:

    IOPar&		pars_;
    Position		pos_;

    bool		leave_;
    bool		rejectOK(CallBacker*);

    friend class	uiVarWizard;

};


#endif

