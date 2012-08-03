#ifndef uicrdevenv_h
#define uicrdevenv_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          Jan 2004
 RCS:           $Id: uicrdevenv.h,v 1.7 2012-08-03 13:00:59 cvskris Exp $
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class uiGenInput;
class uiFileInput;

mClass(uiIo) uiCrDevEnv : public uiDialog
{
public:

    static void		crDevEnv(uiParent*);		
    static bool		isOK(const char* dir=0); //!< default dir: $WORK

protected:
			uiCrDevEnv(uiParent*,const char*,const char*);

    uiGenInput*		workdirfld;
    uiFileInput*	basedirfld;

    bool		acceptOK(CallBacker*);
};

#endif

