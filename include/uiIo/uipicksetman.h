#ifndef uipicksetman_h
#define uipicksetman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uipicksetman.h,v 1.2 2007-05-25 10:16:34 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

/*! \brief
PickSet manager
*/

class uiPickSetMan : public uiObjFileMan
{
public:
    				uiPickSetMan(uiParent*);
				~uiPickSetMan();

protected:

    void			mkFileInfo();
    void			mergeSets(CallBacker*);

};

#endif
