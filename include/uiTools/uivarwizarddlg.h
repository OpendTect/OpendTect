#ifndef uivarwizarddlg_h
#define uivarwizarddlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id: uivarwizarddlg.h,v 1.1 2010-02-17 16:11:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"


/*!\brief Dialog in flexible wizard series. */

mClass uiVarWizardDlg : public uiDialog
{
public:

    enum Position	{ Start, Middle, End };

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
