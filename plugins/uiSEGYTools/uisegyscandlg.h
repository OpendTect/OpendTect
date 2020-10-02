#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2008
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uisegyreaddlg.h"
namespace SEGY { class Scanner; class FileIndexer; }
class uiSeisSel;
class uiSeis2DLineNameSel;
class uiBatchJobDispatcherSel;


/*!\brief Dialog to import SEG-Y files after basic setup. */

mExpClass(uiSEGYTools) uiSEGYScanDlg : public uiSEGYReadDlg
{ mODTextTranslationClass(uiSEGYScanDlg);
public :

			uiSEGYScanDlg(uiParent*,const Setup&,IOPar&,
				      bool forsurvsetup);
			~uiSEGYScanDlg();

    SEGY::Scanner*	getScanner();	//!< becomes yours

    virtual DBKey	outputID() const;

protected:

    SEGY::FileIndexer*	indexer_;
    SEGY::Scanner*	scanner_;
    bool		forsurvsetup_;

    uiSeisSel*		outfld_;

    uiBatchJobDispatcherSel*	batchfld_;
    uiSeis2DLineNameSel*	lnmfld_;

    virtual bool	doWork(const IOObj&);

};
