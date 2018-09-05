#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2018
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiioobjsel.h"

mExpClass(uiIo) uiGeom2DSel : public uiIOObjSel
{
public:
                        uiGeom2DSel(uiParent*,bool forread,
                                const uiString& seltxt=uiString::empty());
			uiGeom2DSel(uiParent*,bool forread,
				    const uiIOObjSel::Setup&);
                        ~uiGeom2DSel();

protected:
};
