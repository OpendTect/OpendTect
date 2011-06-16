#ifndef uimanprops_h
#define uimanprops_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2011
 RCS:           $Id: uimanprops.h,v 1.1 2011-06-16 15:10:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class PropertyRefSet;
class uiBuildPROPS;
class uiGenInput;


mClass uiManPROPS : public uiDialog
{
public:

    			uiManPROPS(uiParent*);

protected:

    uiBuildPROPS*	buildfld_;
    uiGenInput*		srcfld_;

    bool		acceptOK(CallBacker*);

};


#endif
