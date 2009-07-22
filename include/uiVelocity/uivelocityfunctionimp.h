#ifndef uivelocityfunctionimp_h
#define uivelocityfunctionimp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Aug 2008
 RCS:		$Id: uivelocityfunctionimp.h,v 1.5 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiFileInput;
class uiIOObjSel;
class CtxtIOObj;
class uiTableImpDataSel;
class uiVelocityDesc;

namespace Table { class FormatDesc; }

namespace Vel
{

mClass uiImportVelFunc : public uiDialog
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

    void                formatSel(CallBacker*);

    virtual bool	acceptOK(CallBacker*);    

};


} //namespace Vel


#endif
