#ifndef uiimppvds_h
#define uiimppvds_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2010
 RCS:           $Id: uiimppvds.h,v 1.2 2010-06-25 11:17:02 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiIOObjSel;
class uiGenInput;
class uiFileInput;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }


mClass uiImpPVDS : public uiDialog 
{
public:
			uiImpPVDS(uiParent*);
			~uiImpPVDS();

protected:

    Table::FormatDesc&	fd_;

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiGenInput*		row1isdatafld_;;
    uiIOObjSel*		outfld_;

    bool		acceptOK(CallBacker*);

};


#endif
