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

mExpClass(uiSEGY) uiSEGYFileSelector : public uiDialog
{ mODTextTranslationClass(uiSEGYFileSelector)
public:
		uiSEGYFileSelector(uiParent*, const char* fnm,
				   const char* vntname);

    void	getSelNames(BufferStringSet&);

protected:
    bool	acceptOK();

    uiListBox*	filenmsfld_;
};
