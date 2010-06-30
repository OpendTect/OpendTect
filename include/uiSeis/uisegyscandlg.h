#ifndef uisegyscandlg_h
#define uisegyscandlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2008
 RCS:           $Id: uisegyscandlg.h,v 1.9 2010-06-30 17:17:28 cvskris Exp $
________________________________________________________________________

-*/

#include "uisegyreaddlg.h"
namespace SEGY { class Scanner; class PreStackIndexer; }
class CtxtIOObj;
class uiSeisSel;
class uiSeis2DLineSel;


/*!\brief Dialog to import SEG-Y files after basic setup. */

mClass uiSEGYScanDlg : public uiSEGYReadDlg
{
public :

			uiSEGYScanDlg(uiParent*,const Setup&,IOPar&,
				      bool forsurvsetup);
			~uiSEGYScanDlg();

    SEGY::Scanner*	getScanner();	//!< becomes yours
    static void		presentReport(uiParent*,const SEGY::Scanner&,
	    			      const char* reportfilenm=0);

protected:

    SEGY::PreStackIndexer*	indexer_;
    SEGY::Scanner*		scanner_;
    bool			forsurvsetup_;
    CtxtIOObj&			ctio_;

    uiSeisSel*			outfld_;
    uiSeis2DLineSel*		lnmfld_;

    virtual bool		doWork(const IOObj&);
};


#endif
