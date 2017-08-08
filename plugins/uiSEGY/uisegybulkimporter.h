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
class uiSEGYImpType;

mExpClass(uiSEGY) uiSEGYBulkImporter : public uiDialog
{ mODTextTranslationClass(uiSEGYBulkImporter)
public:
			uiSEGYBulkImporter(uiParent*);

protected:
    bool		selectVintage();
    void		addCB(CallBacker*);
    void		removeCB(CallBacker*);
    void		selectFilesCB(CallBacker*);
    void		fillRow( int rowid);
    bool		acceptOK();

    BufferStringSet	selfilenms_;
    uiTable*		table_;
    uiSEGYImpType*	imptypefld_;
    BufferString	vintagenm_;
};
