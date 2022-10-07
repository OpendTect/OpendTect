#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiioobjsel.h"

mExpClass(uiIo) uiPointSetPolygonSel : public uiIOObjSel
{
public:
			~uiPointSetPolygonSel();

    static uiPointSetPolygonSel*
			create(uiParent*,bool forread,bool ispoly,
				const uiString& seltxt=uiString::emptyString());
    static uiPointSetPolygonSel*
			create(uiParent*,bool forread,bool ispoly,
			       const uiIOObjSel::Setup&);

protected:
			uiPointSetPolygonSel(uiParent*,bool forread,
					     bool ispoly,
					     const uiString& seltxt=
						uiString::emptyString());
			uiPointSetPolygonSel(uiParent*,bool forread,bool ispoly,
					     const uiIOObjSel::Setup&);
};


mExpClass(uiIo) uiPointSetSel : public uiPointSetPolygonSel
{
public:
			uiPointSetSel(uiParent*,bool forread,
				      const uiString& seltxt=
						uiString::emptyString());
			uiPointSetSel(uiParent*,bool forread,
				    const uiIOObjSel::Setup&);
			~uiPointSetSel();

protected:
};


mExpClass(uiIo) uiPolygonSel : public uiPointSetPolygonSel
{
public:
			uiPolygonSel(uiParent*,bool forread,
				     const uiString& seltxt=
						uiString::emptyString());
			uiPolygonSel(uiParent*,bool forread,
				    const uiIOObjSel::Setup&);
			~uiPolygonSel();

protected:
};
