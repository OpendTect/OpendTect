#ifndef uisegyimpdlg_h
#define uisegyimpdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyimpdlg.h,v 1.6 2008-10-01 11:41:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uisegyread.h"
class uiSeisSel;
class IOObj;
class CtxtIOObj;
class uiCheckBox;
class uiGenInput;
class uiSeisTransfer;
class uiSEGYFileOpts;


/*!\brief Dialog to import SEG-Y files after basic setup. */

class uiSEGYImpDlg : public uiDialog
{
public :

    class Setup : public uiDialog::Setup
    {
    public:

    			Setup(Seis::GeomType);

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(int,nrexamine)		// default 0=no examine
	mDefSetupMemb(uiSEGYRead::RevType,rev)	// default Rev0
    };

			uiSEGYImpDlg(uiParent*,const Setup&,IOPar&);
			~uiSEGYImpDlg();

    void		use(const IOObj*,bool force);
    void		updatePars()		{ getParsFromScreen(true); }
    const char*		saveObjName() const;

    Notifier<uiSEGYImpDlg> readParsReq;
    Notifier<uiSEGYImpDlg> writeParsReq;

protected:

    const Setup		setup_;
    IOPar&		pars_;
    CtxtIOObj&		ctio_;

    uiSEGYFileOpts*	optsfld_;
    uiGenInput*		savesetupfld_;
    uiSeisTransfer*	transffld_;
    uiSeisSel*		seissel_;
    uiCheckBox*		morebut_;

    bool		getParsFromScreen(bool);
    void		setupWin(CallBacker*);
    void		readParsCB(CallBacker*);

    friend class	uiSEGYImpSimilarDlg;
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);
    bool		doWork(const IOObj&,const IOObj&,
	    			const char*,const char*);

};


#endif
