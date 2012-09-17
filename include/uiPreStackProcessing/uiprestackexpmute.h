
#ifndef uiprestackexpmute_h
#define uiprestackexpmute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		June 2008
 RCS:		$Id: uiprestackexpmute.h,v 1.3 2009/07/22 16:01:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;

namespace PreStack
{

mClass uiExportMute : public uiDialog
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
