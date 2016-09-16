#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2016
-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class uiFileInput;

mExpClass(uiEarthModel) uiBulkHorizonExport : public uiDialog
{ mODTextTranslationClass(uiBulkHorizonExport);
public:
			uiBulkHorizonExport(uiParent*);
			~uiBulkHorizonExport();

protected:

    bool		acceptOK();

    uiFileInput*	outfld_;
};
