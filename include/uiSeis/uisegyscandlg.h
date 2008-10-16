#ifndef uisegyscandlg_h
#define uisegyscandlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2008
 RCS:           $Id: uisegyscandlg.h,v 1.1 2008-10-16 16:31:59 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyreaddlg.h"
namespace SEGY { class Scanner; }


/*!\brief Dialog to import SEG-Y files after basic setup. */

class uiSEGYScanDlg : public uiSEGYReadDlg
{
public :

			uiSEGYScanDlg(uiParent*,const Setup&,IOPar&);
			~uiSEGYScanDlg();

    SEGY::Scanner*	getScanner();	//!< becomes yours

protected:

    SEGY::Scanner*	scanner_;

    virtual bool	doWork(const IOObj&);

};


#endif
