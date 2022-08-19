#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uivarwizarddlg.h"
#include "uisegyread.h"
class uiSEGYFileOpts;


/*!\brief Dialog to import SEG-Y files after basic setup. */

mExpClass(uiSEGYTools) uiSEGYReadDlg : public uiVarWizardDlg
{ mODTextTranslationClass(uiSEGYReadDlg);
public :

    mExpClass(uiSEGYTools) Setup : public uiDialog::Setup
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

    Notifier<uiSEGYReadDlg> readParsReq;
    Notifier<uiSEGYReadDlg> writeParsReq;
    Notifier<uiSEGYReadDlg> preScanReq;

    virtual MultiID	outputID() const	= 0;

protected:

    const Setup		setup_;

    uiSEGYFileOpts*	optsfld_;

    void		initWin(CallBacker*);
    void		readParsCB(CallBacker*);
    void		writeParsCB(CallBacker*);
    void		preScanCB(CallBacker*);

    friend class	uiSEGYImpSimilarDlg;
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

    bool		getParsFromScreen(bool);
    virtual bool	doWork(const IOObj&)		= 0;

};
