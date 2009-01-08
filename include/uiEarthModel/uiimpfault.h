#ifndef uiimpfault_h
#define uiimpfault_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimpfault.h,v 1.8 2009-01-08 07:32:45 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiTableImpDataSel;

namespace EM { class Fault3D; }
namespace Table { class FormatDesc; }

/*! \brief Dialog for fault import */

mClass uiImportFault : public uiDialog
{
public:
			uiImportFault(uiParent*);
			~uiImportFault();

protected:

    uiFileInput*	infld_;
    uiFileInput*	formatfld_;
    uiGenInput*		typefld_;
    uiIOObjSel*		outfld_;

    void		typeSel(CallBacker*);
    virtual bool	acceptOK(CallBacker*);
    bool		checkInpFlds();
    bool		handleAscii();
    bool		handleLMKAscii();
    EM::Fault3D*	createFault() const;

    CtxtIOObj&		ctio_;

    Table::FormatDesc&	fd_;
    uiTableImpDataSel*	dataselfld_;
};


#endif
