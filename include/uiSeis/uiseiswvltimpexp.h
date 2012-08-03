#ifndef uiseiswvltimpexp_h
#define uiseiswvltimpexp_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006 / Dec 2009
 RCS:           $Id: uiseiswvltimpexp.h,v 1.2 2012-08-03 13:01:09 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "multiid.h"

class CtxtIOObj;
class uiGenInput;
class uiIOObjSel;
class uiFileInput;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }


mClass(uiSeis) uiSeisWvltImp : public uiDialog
{
public:
			uiSeisWvltImp(uiParent*);
			~uiSeisWvltImp();

    MultiID		selKey() const;

protected:

    CtxtIOObj&		ctio_;
    Table::FormatDesc&	fd_;

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiGenInput*		scalefld_;
    uiIOObjSel*		wvltfld_;

    bool		acceptOK(CallBacker*);

};


mClass(uiSeis) uiSeisWvltExp : public uiDialog
{
public:
			uiSeisWvltExp(uiParent*);

protected:

    uiIOObjSel*		wvltfld_;
    uiFileInput*	outpfld_;
    uiGenInput*		addzfld_;

    bool		acceptOK(CallBacker*);

};


#endif

