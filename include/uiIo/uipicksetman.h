#ifndef uipicksetman_h
#define uipicksetman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uipicksetman.h,v 1.1 2006-08-03 18:56:52 cvsnanne Exp $
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
};

#endif
