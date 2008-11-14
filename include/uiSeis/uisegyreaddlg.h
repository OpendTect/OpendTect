#ifndef uisegyreaddlg_h
#define uisegyreaddlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2008
 RCS:           $Id: uisegyreaddlg.h,v 1.3 2008-11-14 14:46:17 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uisegyread.h"
class IOObj;
class uiGenInput;
class uiSEGYFileOpts;


/*!\brief Dialog to import SEG-Y files after basic setup. */

class uiSEGYReadDlg : public uiDialog
{
public :

    class Setup : public uiDialog::Setup
    {
    public:

    			Setup(Seis::GeomType);

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(uiSEGYRead::RevType,rev)	// default Rev0
    };

			uiSEGYReadDlg(uiParent*,const Setup&,IOPar&);
			~uiSEGYReadDlg();

    void		updatePars()		{ getParsFromScreen(true); }
    virtual void	use(const IOObj*,bool force);
    const char*		saveObjName() const;

    Notifier<uiSEGYReadDlg> readParsReq;
    Notifier<uiSEGYReadDlg> writeParsReq;
    Notifier<uiSEGYReadDlg> preScanReq;

protected:

    const Setup		setup_;
    IOPar&		pars_;
    uiGroup*		optsgrp_;

    uiSEGYFileOpts*	optsfld_;
    uiGenInput*		savesetupfld_;

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
