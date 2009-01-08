
#ifndef uivelocityfunctionimp_h
#define uivelocityfunctionimp_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Aug 2008
 RCS:		$Id: uivelocityfunctionimp.h,v 1.3 2009-01-08 08:37:00 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiFileInput;
class uiIOObjSel;
class CtxtIOObj;
class uiTableImpDataSel;
class uiComboBox;

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
    uiComboBox*	typefld_;
    uiIOObjSel*		outfld_;

    CtxtIOObj&          ctio_;

    Table::FormatDesc&	fd_;
    uiTableImpDataSel*	dataselfld_;

    void                formatSel(CallBacker*);

    virtual bool	acceptOK(CallBacker*);    

};


} //namespace Vel


#endif
