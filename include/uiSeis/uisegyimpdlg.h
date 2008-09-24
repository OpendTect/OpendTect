#ifndef uisegyimpdlg_h
#define uisegyimpdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyimpdlg.h,v 1.1 2008-09-24 19:48:37 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "seistype.h"
class uiSEGYFileOpts;


/*!\brief Dialog to import SEG-Y files after basic setup. */

class uiSEGYImpDlg : public uiDialog
{
public :

    class Setup : public uiDialog::Setup
    {
    public:

    			Setup( Seis::GeomType gt )
			    : uiDialog::Setup("SEG-Y Import",
				    	      mNoDlgTitle, "103.1.5")
			    , geom_(gt)	
			    , nrexamine_(0)	
			    , isrev1_(false)		{}

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(int,nrexamine)
	mDefSetupMemb(bool,isrev1)
    };

			uiSEGYImpDlg(uiParent*,const Setup&,IOPar&);
			~uiSEGYImpDlg();

protected:

    const Setup		setup_;
    IOPar&		pars_;

    uiSEGYFileOpts*	optsfld_;
    uiToolBar*		uitb_;

    bool		getFromScreen(bool);
    void		setupWin(CallBacker*);
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
