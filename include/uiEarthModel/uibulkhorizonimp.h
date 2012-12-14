#ifndef uibulkhorizonimp_h
#define uibulkhorizonimp_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2012
 * ID       : $Id$
-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class uiFileInput;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }

mClass(uiEarthModel) uiBulkHorizonImport : public uiDialog
{
public:
			uiBulkHorizonImport(uiParent*);
			~uiBulkHorizonImport();

protected:

    bool		acceptOK(CallBacker*);

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    Table::FormatDesc*	fd_;
};

#endif

