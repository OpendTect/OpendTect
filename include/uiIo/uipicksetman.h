#ifndef uipicksetman_h
#define uipicksetman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uipicksetman.h,v 1.6 2010-11-09 04:41:37 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

/*! \brief
PickSet manager
*/

class uiButton;

mClass uiPickSetMan : public uiObjFileMan
{
public:
    				uiPickSetMan(uiParent*);
				~uiPickSetMan();

    static Notifier<uiPickSetMan>* fieldsCreated();

protected:

    void			mkFileInfo();
    void			mergeSets(CallBacker*);

};

#endif
