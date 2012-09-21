
#ifndef uiprestackimpmute_h
#define uiprestackimpmut _h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		June 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"

class uiFileInput;
class CtxtIOObj;
class uiIOObjSel;
class uiGenInput;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }

namespace PreStack
{
mClass(uiPreStackProcessing) uiImportMute : public uiDialog
{
public:
  			uiImportMute(uiParent*);
		    	~uiImportMute();

protected:
    
    uiFileInput*	inpfld_;
    uiGenInput*		inpfilehaveposfld_;
    uiGenInput*		posdatainfld_;
    uiGenInput*		inlcrlfld_;
    uiIOObjSel*		outfld_;

    CtxtIOObj&		ctio_;
    
    Table::FormatDesc&	fd_;
    uiTableImpDataSel*	dataselfld_;

    bool		haveInpPosData() const;
    
    void 		formatSel(CallBacker*);
    void		changePrefPosInfo(CallBacker*);

    virtual bool	acceptOK(CallBacker*);    
};


}; //namespace Prestack
#endif

