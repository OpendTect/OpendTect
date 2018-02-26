#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uiobjfileman.h"

/*! \brief 'Manage Probability Density Functions' */

mExpClass(uiIo) uiProbDenFuncMan : public uiObjFileMan
{ mODTextTranslationClass(uiProbDenFuncMan);
public:
			uiProbDenFuncMan(uiParent*);
			~uiProbDenFuncMan();

protected:

    void		browsePush(CallBacker*);
    void		genPush(CallBacker*);

    virtual bool	gtItemInfo(const IOObj&,uiPhraseSet&) const;

};
