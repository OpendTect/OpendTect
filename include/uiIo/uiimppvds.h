#ifndef uiimppvds_h
#define uiimppvds_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
class IOObj;
class uiIOObjSel;
class uiGenInput;
class uiFileInput;
class DataPointSet;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }


mClass(uiIo) uiImpPVDS : public uiDialog 
{
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

    bool		acceptOK(CallBacker*);

    bool		getData(std::istream&,Table::FormatDesc&,DataPointSet&);
    bool		writeData(const DataPointSet&,const IOObj&);

};


#endif

