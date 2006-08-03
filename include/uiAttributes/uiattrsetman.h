#ifndef uiattrsetman_h
#define uiattrsetman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiattrsetman.h,v 1.1 2006-08-03 18:57:46 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

/*! \brief
AttributeSet manager
*/

class uiAttrSetMan : public uiObjFileMan
{
public:
    				uiAttrSetMan(uiParent*);
				~uiAttrSetMan();

protected:

    void			mkFileInfo();
};

#endif
