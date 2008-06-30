
#ifndef uiprestackexpmute_h
#define uiprestackexpmute_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2008
 RCS:		$Id: uiprestackexpmute.h,v 1.1 2008-06-30 06:15:44 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;

namespace PreStack
{

class uiExportMute : public uiDialog
{
public:
			uiExportMute(uiParent*);
		    	~uiExportMute();

protected:

    uiIOObjSel*		infld_;
    uiGenInput*		coordfld_;
    uiFileInput*	outfld_;

    CtxtIOObj&		ctio_;

    virtual bool	acceptOK(CallBacker*);
    bool		writeAscii();    
};

} //namespace PreStack


#endif
