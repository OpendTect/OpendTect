#ifndef uiattrsetman_h
#define uiattrsetman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiattrsetman.h,v 1.6 2011/09/16 10:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

class uiButton;

/*! \brief
AttributeSet manager
*/

mClass uiAttrSetMan : public uiObjFileMan
{
public:
    				uiAttrSetMan(uiParent*);
				~uiAttrSetMan();

    mDeclInstanceCreatedNotifierAccess(uiAttrSetMan);

protected:

    void			mkFileInfo();
};

#endif
