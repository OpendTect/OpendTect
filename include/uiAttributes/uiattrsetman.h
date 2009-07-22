#ifndef uiattrsetman_h
#define uiattrsetman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiattrsetman.h,v 1.3 2009-07-22 16:01:20 cvsbert Exp $
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
