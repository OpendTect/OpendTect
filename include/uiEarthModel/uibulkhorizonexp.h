#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2016
-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class uiFileSel;

mExpClass(uiEarthModel) uiBulkHorizonExport : public uiDialog
{ mODTextTranslationClass(uiBulkHorizonExport);
public:
			uiBulkHorizonExport(uiParent*);
			~uiBulkHorizonExport();

protected:

    bool		acceptOK();

    uiFileSel*		outfld_;

};
