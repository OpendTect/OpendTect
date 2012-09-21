#ifndef uisegyreaddlg_h
#define uisegyreaddlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uivarwizarddlg.h"
#include "uisegyread.h"
class IOObj;
class uiGenInput;
class uiSEGYFileOpts;


/*!\brief Dialog to import SEG-Y files after basic setup. */

mClass(uiSeis) uiSEGYReadDlg : public uiVarWizardDlg
{
public :

    mClass(uiSeis) Setup : public uiDialog::Setup
    {
    public:

    			Setup(Seis::GeomType);

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(uiSEGYRead::RevType,rev)	// default Rev0
    };

			uiSEGYReadDlg(uiParent*,const Setup&,IOPar&,
					bool forsurvsetup=false);

    void		updatePars()		{ getParsFromScreen(true); }
    virtual void	use(const IOObj*,bool force);
    const char*		saveObjName() const;

    Notifier<uiSEGYReadDlg> readParsReq;
    Notifier<uiSEGYReadDlg> writeParsReq;
    Notifier<uiSEGYReadDlg> preScanReq;

protected:

    const Setup		setup_;
    uiGroup*		optsgrp_;

    uiSEGYFileOpts*	optsfld_;
    uiGenInput*		savesetupfld_;

    void		initWin(CallBacker*);
    void		readParsCB(CallBacker*);
    void		preScanCB(CallBacker*);

    friend class	uiSEGYImpSimilarDlg;
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

    bool		getParsFromScreen(bool);
    bool		displayWarnings(const BufferStringSet&,
	    				bool withstop=false);
    virtual bool	doWork(const IOObj&)		= 0;

};


#endif

