#ifndef uivarwizarddlg_h
#define uivarwizarddlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id: uivarwizarddlg.h,v 1.4 2012-08-03 13:01:16 cvskris Exp $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
class IOPar;


/*!\brief Dialog in flexible wizard series. */

mClass(uiTools) uiVarWizardDlg : public uiDialog
{
public:

    enum Position	{ Start, Middle, End };

    			uiVarWizardDlg(uiParent*,const uiDialog::Setup&,IOPar&,
					Position);
			~uiVarWizardDlg();

    static const char*	sProceedButTxt();
    static const char*	sBackButTxt();

protected:

    IOPar&		pars_;
    Position		pos_;

    bool		leave_;
    bool		rejectOK(CallBacker*);

    friend class	uiVarWizard;

};


#endif

