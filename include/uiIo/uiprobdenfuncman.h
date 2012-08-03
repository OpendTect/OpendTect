#ifndef uiprobdenfuncman_h
#define uiprobdenfuncman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
 RCS:           $Id: uiprobdenfuncman.h,v 1.3 2012-08-03 13:01:01 cvskris Exp $
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiobjfileman.h"

/*! \brief
Probability Density Function manager
*/

mClass(uiIo) uiProbDenFuncMan : public uiObjFileMan
{
public:
    				uiProbDenFuncMan(uiParent*);
				~uiProbDenFuncMan();

protected:

    void			browsePush(CallBacker*);

    void			mkFileInfo();

};

#endif

