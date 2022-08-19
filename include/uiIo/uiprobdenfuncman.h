#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
