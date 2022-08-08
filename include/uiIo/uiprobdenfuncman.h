#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiobjfileman.h"

/*! \brief
Probability Density Function manager
*/

mExpClass(uiIo) uiProbDenFuncMan : public uiObjFileMan
{ mODTextTranslationClass(uiProbDenFuncMan);
public:
				uiProbDenFuncMan(uiParent*);
				~uiProbDenFuncMan();

protected:

    void			initDlg() override;
    void			browsePush(CallBacker*);
    void			genPush(CallBacker*);

    void			mkFileInfo() override;

};

