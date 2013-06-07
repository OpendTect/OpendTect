#ifndef uiprestackimpevent_h
#define uiprestackimpevent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2011
 RCS:           $Id$
________________________________________________________________________

-*/


#include "uidialog.h"

class uiFileInput;
class uiIOObjSel;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }

namespace PreStack
{


mClass uiEventImport : public uiDialog
{
public:
    			uiEventImport(uiParent*);

protected:
    bool		acceptOK(CallBacker*);

    Table::FormatDesc&	fd_;

    uiFileInput*	filefld_;
    uiTableImpDataSel*	dataselfld_;
    uiIOObjSel*		outputfld_;
};


}; //namespace

#endif
