#ifndef uipicksetman_h
#define uipicksetman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiobjfileman.h"

/*! \brief
PickSet manager
*/

class uiToolButton;

mExpClass(uiIo) uiPickSetMan : public uiObjFileMan
{ mODTextTranslationClass(uiPickSetMan);
public:
    				uiPickSetMan(uiParent*);
				~uiPickSetMan();

    mDeclInstanceCreatedNotifierAccess(uiPickSetMan);

protected:
    uiToolButton*		mergebut_;

    void			ownSelChg();
    void			mkFileInfo();
    void			mergeSets(CallBacker*);

};

#endif
