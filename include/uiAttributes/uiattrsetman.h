#ifndef uiattrsetman_h
#define uiattrsetman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiattrsetman.h,v 1.7 2012-08-03 13:00:48 cvskris Exp $
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiobjfileman.h"

class uiButton;

/*! \brief
AttributeSet manager
*/

mClass(uiAttributes) uiAttrSetMan : public uiObjFileMan
{
public:
    				uiAttrSetMan(uiParent*);
				~uiAttrSetMan();

    mDeclInstanceCreatedNotifierAccess(uiAttrSetMan);

protected:

    void			mkFileInfo();
};

#endif

