#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2010
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"
#include "od_iosfwd.h"

class IOObj;
class uiIOObjSel;
class uiGenInput;
class uiFileSel;
class DataPointSet;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }


mExpClass(uiIo) uiImpPVDS : public uiDialog
{ mODTextTranslationClass(uiImpPVDS);
public:
			uiImpPVDS(uiParent*,bool is2d=false);
			~uiImpPVDS();

protected:

    Table::FormatDesc&	fd_;
    const bool		is2d_;

    uiFileSel*		inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiGenInput*		row1isdatafld_;
    uiIOObjSel*		outfld_;

    void		inpChgd(CallBacker*);
    bool		acceptOK();

    bool		getData(od_istream&,Table::FormatDesc&,DataPointSet&);
    bool		writeData(const DataPointSet&,const IOObj&);
};
