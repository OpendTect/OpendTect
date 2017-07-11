#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		July 2017
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"

class uiTable;

mExpClass(uiSEGY) uiSEGYBulkImporter : public uiDialog
{ mODTextTranslationClass(uiSEGYBulkImporter)
public:
			uiSEGYBulkImporter(uiParent*,const BufferStringSet&);

protected:
    void		fillTable();
    void		advanceCB(CallBacker*);

    const BufferStringSet&	selfilenms_;
    uiTable*			bulktable_;

};
