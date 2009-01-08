#ifndef uipicksetman_h
#define uipicksetman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uipicksetman.h,v 1.3 2009-01-08 07:23:07 cvsranojay Exp $
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
