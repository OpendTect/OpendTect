#ifndef uiprobdenfuncman_h
#define uiprobdenfuncman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
 RCS:           $Id: uiprobdenfuncman.h,v 1.1 2010-01-28 09:47:27 cvsnanne Exp $
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

    void			mkFileInfo();

};

#endif
