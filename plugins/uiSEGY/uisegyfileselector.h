#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		August 2017
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"

class uiListBox;
class uiTextEdit;
namespace SEGY{ class ImpType; }

mExpClass(uiSEGY) uiSEGYFileSelector : public uiDialog
{ mODTextTranslationClass(uiSEGYFileSelector)
public:
		uiSEGYFileSelector(uiParent*, const char* fnm,
				   const char* vntname,
				   const SEGY::ImpType& imptype);

    void	getSelNames(BufferStringSet&);

protected:
    void	selChgCB(CallBacker*);
    void	quickScanCB(CallBacker*);
    bool	acceptOK();

    uiListBox*		filenmsfld_;
    uiTextEdit*		txtfld_;
    BufferString	dirnm_;
    const SEGY::ImpType& imptype_;
};
