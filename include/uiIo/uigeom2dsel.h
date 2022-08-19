#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiioobjsel.h"

mExpClass(uiIo) uiGeom2DSel : public uiIOObjSel
{
public:
			uiGeom2DSel(uiParent*,bool forread,
				const uiString& seltxt=uiString::emptyString());
			uiGeom2DSel(uiParent*,bool forread,
				    const uiIOObjSel::Setup&);
			~uiGeom2DSel();

protected:
};
