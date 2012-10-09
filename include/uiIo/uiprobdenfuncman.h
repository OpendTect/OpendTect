#ifndef uiprobdenfuncman_h
#define uiprobdenfuncman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiobjfileman.h"

/*! \brief
Probability Density Function manager
*/

mClass uiProbDenFuncMan : public uiObjFileMan
{
public:
    				uiProbDenFuncMan(uiParent*);
				~uiProbDenFuncMan();

protected:

    void			browsePush(CallBacker*);

    void			mkFileInfo();

};

#endif
