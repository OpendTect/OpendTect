#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    virtual MultiID	outputID() const;

protected:

    SEGY::FileIndexer*	indexer_;
    SEGY::Scanner*	scanner_;
    bool		forsurvsetup_;

    uiSeisSel*		outfld_;

    uiBatchJobDispatcherSel*	batchfld_;
    uiSeis2DLineNameSel*	lnmfld_;

    virtual bool	doWork(const IOObj&);

};
