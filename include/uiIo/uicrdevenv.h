#ifndef uicrdevenv_h
#define uicrdevenv_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          Jan 2004
 RCS:           $Id: uicrdevenv.h,v 1.3 2004-01-23 09:39:41 dgb Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiGenInput;
class uiFileInput;

class uiCrDevEnv : public uiDialog
{
public:

    static void		crDevEnv( uiParent* );		
    static bool		isOK( const char* dir=0 ); //!< default dir: $WORK

protected:
			uiCrDevEnv( uiParent*, const char*, const char*,
				    const char*);

    uiGenInput*		workdirfld;
    uiFileInput*	basedirfld;

    bool		acceptOK(CallBacker*);
};

#endif
