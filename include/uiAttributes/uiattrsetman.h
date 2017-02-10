#ifndef uiattrsetman_h
#define uiattrsetman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiobjfileman.h"

class uiListBox;


/*!
\brief AttributeSet Manager
*/

mExpClass(uiAttributes) uiAttrSetMan : public uiObjFileMan
{ mODTextTranslationClass(uiAttrSetMan);
public:
    				uiAttrSetMan(uiParent*);
				~uiAttrSetMan();

    mDeclInstanceCreatedNotifierAccess(uiAttrSetMan);

protected:

    void			mkFileInfo();

    uiListBox*			attribfld_;
};

#endif
