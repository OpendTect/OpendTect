
#ifndef uiprestackimpmute_h
#define uiprestackimpmut _h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2008
 RCS:		$Id: uiprestackimpmute.h,v 1.1 2008-06-23 06:50:47 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "mathfunc.h"

class uiFileInput;
class CtxtIOObj;
class uiIOObjSel;
class uiGenInput;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }

class uiImportMute : public uiDialog
{
public:
  			uiImportMute(uiParent*);
		    	~uiImportMute();

protected:
    
    uiFileInput*	inpfld_;
    uiGenInput*		extptypefld_;
    uiIOObjSel*		outfld_;

    CtxtIOObj&		ctio_;
    
    Table::FormatDesc&	fd_;
    uiTableImpDataSel*	dataselfld_;

    PointBasedMathFunction::InterpolType	getInterpolType();

    virtual bool	acceptOK( CallBacker* );    
};

#endif
