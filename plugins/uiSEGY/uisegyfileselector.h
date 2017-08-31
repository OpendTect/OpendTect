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
namespace File{ class Path; }

mExpClass(uiSEGY) uiSEGYFileSelector : public uiDialog
{ mODTextTranslationClass(uiSEGYFileSelector)
public:
		uiSEGYFileSelector(uiParent*, const char* fnm,
				   const char* vntname,
				   const SEGY::ImpType& imptype);
		~uiSEGYFileSelector();

    void	getSelNames(BufferStringSet&);

protected:
    void	selChgCB(CallBacker*);
    void	quickScanCB(CallBacker*);
    bool	acceptOK();
    bool	rejectOK();

    uiListBox*		filenmsfld_;
    uiTextEdit*		txtfld_;
    const File::Path&	fp_;
    const SEGY::ImpType& imptype_;
};
