#ifndef uiattrsetman_h
#define uiattrsetman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiattrsetman.h,v 1.5 2010-11-09 04:41:37 cvsnanne Exp $
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

    static Notifier<uiAttrSetMan>* fieldsCreated();

protected:

    void			mkFileInfo();
};

#endif
