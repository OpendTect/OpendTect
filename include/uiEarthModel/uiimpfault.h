#ifndef uiimpfault_h
#define uiimpfault_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimpfault.h,v 1.4 2008-05-21 06:30:38 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }

/*! \brief Dialog for fault selection */

class uiImportLMKFault : public uiDialog
{
public:
			uiImportLMKFault(uiParent*);
			~uiImportLMKFault();

protected:

    uiFileInput*	infld;
    uiFileInput*	formatfilefld;
    uiIOObjSel*		outfld;

    virtual bool	acceptOK(CallBacker*);
    bool		checkInpFlds();
    bool		handleAscii();

    CtxtIOObj&		ctio;

    Table::FormatDesc&	fd_;
    uiTableImpDataSel*	dataselfld_;

};


#endif
