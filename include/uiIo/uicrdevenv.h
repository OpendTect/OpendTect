#ifndef uicrdevenv_h
#define uicrdevenv_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          Jan 2004
 RCS:           $Id: uicrdevenv.h,v 1.6 2009/07/22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiFileInput;

mClass uiCrDevEnv : public uiDialog
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
