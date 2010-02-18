#ifndef uivarwizarddlg_h
#define uivarwizarddlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id: uivarwizarddlg.h,v 1.2 2010-02-18 12:09:48 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class IOPar;


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
