#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          Jan 2004
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"

class uiGenInput;
class uiFileSel;

mExpClass(uiIo) uiCrDevEnv : public uiDialog
{ mODTextTranslationClass(uiCrDevEnv);
public:

    static void		crDevEnv(uiParent*);
    static bool		isOK(const char* dir=0); //!< default dir: $WORK

protected:
			uiCrDevEnv(uiParent*,const char*,const char*);

    uiGenInput*		workdirfld;
    uiFileSel*		basedirfld;

    bool		acceptOK();

};
