
#ifndef uiprestackexpmute_h
#define uiprestackexpmute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		June 2008
 RCS:		$Id: uiprestackexpmute.h,v 1.4 2012-08-03 13:01:05 cvskris Exp $
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"

class CtxtIOObj;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;

namespace PreStack
{

mClass(uiPreStackProcessing) uiExportMute : public uiDialog
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

