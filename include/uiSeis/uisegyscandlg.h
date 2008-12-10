#ifndef uisegyscandlg_h
#define uisegyscandlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2008
 RCS:           $Id: uisegyscandlg.h,v 1.5 2008-12-10 16:16:33 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyreaddlg.h"
namespace SEGY { class Scanner; }
class CtxtIOObj;
class uiSeisSel;
class uiSeisLineSel;


/*!\brief Dialog to import SEG-Y files after basic setup. */

class uiSEGYScanDlg : public uiSEGYReadDlg
{
public :

			uiSEGYScanDlg(uiParent*,const Setup&,IOPar&,
				      bool forsurvsetup);
			~uiSEGYScanDlg();

    SEGY::Scanner*	getScanner();	//!< becomes yours
    static void		presentReport(uiParent*,const SEGY::Scanner&,
	    			      const char* reportfilenm=0);

protected:

    SEGY::Scanner*	scanner_;
    bool		forsurvsetup_;
    CtxtIOObj&		ctio_;

    uiSeisSel*		outfld_;
    uiSeisLineSel*	lnmfld_;

    virtual bool	doWork(const IOObj&);
    bool		mkOutput(const char*,const char*);

};


#endif
