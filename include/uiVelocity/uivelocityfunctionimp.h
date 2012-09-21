#ifndef uivelocityfunctionimp_h
#define uivelocityfunctionimp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Aug 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uivelocitymod.h"
#include "uidialog.h"

class uiFileInput;
class uiIOObjSel;
class CtxtIOObj;
class uiTableImpDataSel;
class uiVelocityDesc;

namespace Table { class FormatDesc; }

namespace Vel
{

mClass(uiVelocity) uiImportVelFunc : public uiDialog
{
public:
    			uiImportVelFunc(uiParent*);
			~uiImportVelFunc();

protected:
	
    uiFileInput*	inpfld_;
    uiVelocityDesc*	typefld_;
    uiIOObjSel*		outfld_;

    CtxtIOObj&          ctio_;

    Table::FormatDesc&	fd_;
    uiTableImpDataSel*	dataselfld_;

    void		velTypeChangeCB(CallBacker*);
    void                formatSel(CallBacker*);

    virtual bool	acceptOK(CallBacker*);    

};


} //namespace Vel


#endif

