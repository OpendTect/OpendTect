
#ifndef uivelocityfunctionimp_h
#define uivelocityfunctionimp_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Aug 2008
 RCS:		$Id: uivelocityfunctionimp.h,v 1.1 2008-08-08 10:13:43 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiFileInput;
class uiIOObjSel;
class CtxtIOObj;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }

namespace Vel
{

class uiImportVelFunc : public uiDialog
{
public:
    			uiImportVelFunc(uiParent*);
			~uiImportVelFunc();

protected:
	
    uiFileInput*	inpfld_;
    uiIOObjSel*		outfld_;

    CtxtIOObj&          ctio_;

    Table::FormatDesc&	fd_;
    uiTableImpDataSel*	dataselfld_;

    void                formatSel(CallBacker*);

    virtual bool	acceptOK(CallBacker*);    

};


} //namespace Vel


#endif
