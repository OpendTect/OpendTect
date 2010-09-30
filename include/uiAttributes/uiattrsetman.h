#ifndef uiattrsetman_h
#define uiattrsetman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiattrsetman.h,v 1.4 2010-09-30 10:03:34 cvsnageswara Exp $
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
    void			addTool(uiButton*);

protected:

    uiButton*			lastexternal_;
    void			mkFileInfo();
};

#endif
