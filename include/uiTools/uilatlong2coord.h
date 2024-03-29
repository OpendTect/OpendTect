#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uicoordsystem.h"

class LatLong2Coord;
class uiGenInput;
class uiLatLongInp;

namespace Coords
{

mExpClass(uiTools) uiUnlocatedXYSystem : public uiCoordSystem
{ mODTextTranslationClass(uiUnlocatedXYSystem);
public:
    mDefaultFactoryInstantiation1Param( uiCoordSystem, uiUnlocatedXYSystem,
			       uiParent*, UnlocatedXY::sFactoryKeyword(),
			       UnlocatedXY::sFactoryDisplayName() );

			~uiUnlocatedXYSystem();

    bool		initFields(const CoordSystem*) override;

protected:
			uiUnlocatedXYSystem(uiParent*);

    uiGenInput*		xyunitfld_;

    bool		acceptOK() override;

};


mExpClass(uiTools) uiAnchorBasedXYSystem : public uiCoordSystem
{ mODTextTranslationClass(uiAnchorBasedXYSystem);
public:
    mDefaultFactoryInstantiation1Param( uiCoordSystem, uiAnchorBasedXYSystem,
			       uiParent*, AnchorBasedXY::sFactoryKeyword(),
			       AnchorBasedXY::sFactoryDisplayName() );

			~uiAnchorBasedXYSystem();

    bool		initFields(const CoordSystem*) override;

protected:
			uiAnchorBasedXYSystem(uiParent*);

    uiGenInput*		coordfld_;
    uiLatLongInp*	latlngfld_;
    uiGenInput*		xyunitfld_;

    bool		acceptOK() override;

};

} // namespace Coords

