#ifndef uipicksetman_h
#define uipicksetman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uipicksetman.h,v 1.4 2009-07-22 16:01:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

/*! \brief
PickSet manager
*/

mClass uiPickSetMan : public uiObjFileMan
{
public:
    				uiPickSetMan(uiParent*);
				~uiPickSetMan();

protected:

    void			mkFileInfo();
    void			mergeSets(CallBacker*);

};

#endif
