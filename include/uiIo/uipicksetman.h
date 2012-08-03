#ifndef uipicksetman_h
#define uipicksetman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uipicksetman.h,v 1.8 2012-08-03 13:01:00 cvskris Exp $
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiobjfileman.h"

/*! \brief
PickSet manager
*/

class uiButton;

mClass(uiIo) uiPickSetMan : public uiObjFileMan
{
public:
    				uiPickSetMan(uiParent*);
				~uiPickSetMan();

    mDeclInstanceCreatedNotifierAccess(uiPickSetMan);

protected:

    void			mkFileInfo();
    void			mergeSets(CallBacker*);

};

#endif

