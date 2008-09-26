#ifndef uisegyimpdlg_h
#define uisegyimpdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyimpdlg.h,v 1.3 2008-09-26 13:40:01 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uisegyread.h"
class uiSeisSel;
class CtxtIOObj;
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

    Notifier<uiSEGYImpDlg> readParsReq;

protected:

    const Setup		setup_;
    IOPar&		pars_;
    CtxtIOObj&		ctio_;

    uiSEGYFileOpts*	optsfld_;
    uiGenInput*		savesetupfld_;
    uiSeisTransfer*	transffld_;
    uiSeisSel*		seissel_;

    bool		getParsFromScreen(bool);
    void		setupWin(CallBacker*);
    void		readParsCB(CallBacker*);
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
