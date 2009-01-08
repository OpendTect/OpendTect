#ifndef uiattrsetman_h
#define uiattrsetman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiattrsetman.h,v 1.2 2009-01-08 08:50:11 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

/*! \brief
AttributeSet manager
*/

mClass uiAttrSetMan : public uiObjFileMan
{
public:
    				uiAttrSetMan(uiParent*);
				~uiAttrSetMan();

protected:

    void			mkFileInfo();
};

#endif
