#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
class IOObj;
class od_istream;
class uiIOObjSel;
class uiGenInput;
class uiFileInput;
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

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiGenInput*		row1isdatafld_;
    uiIOObjSel*		outfld_;

    void		inputChgd(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    bool		getData(od_istream&,Table::FormatDesc&,DataPointSet&);
    bool		writeData(const DataPointSet&,const IOObj&);

};
