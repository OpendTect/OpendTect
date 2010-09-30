#ifndef uipicksetman_h
#define uipicksetman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uipicksetman.h,v 1.5 2010-09-30 10:03:34 cvsnageswara Exp $
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
    void			addTool(uiButton*);

protected:

    uiButton*			lastexternal_;

    void			mkFileInfo();
    void			mergeSets(CallBacker*);

};

#endif
