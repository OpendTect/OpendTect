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

			uiUnlocatedXYSystem(uiParent*);

    bool		initFields(const CoordSystem*) override;

protected:

    uiGenInput*		xyunitfld_;

    bool		acceptOK() override;

};


mExpClass(uiTools) uiAnchorBasedXYSystem : public uiCoordSystem
{ mODTextTranslationClass(uiAnchorBasedXYSystem);
public:
    mDefaultFactoryInstantiation1Param( uiCoordSystem, uiAnchorBasedXYSystem,
			       uiParent*, AnchorBasedXY::sFactoryKeyword(),
			       AnchorBasedXY::sFactoryDisplayName() );

			uiAnchorBasedXYSystem(uiParent*);

    bool		initFields(const CoordSystem*) override;

protected:

    uiGenInput*		coordfld_;
    uiLatLongInp*	latlngfld_;
    uiGenInput*		xyunitfld_;

    bool		acceptOK() override;

};

} // namespace Coords

mExpClass(uiTools) uiLatLong2CoordDlg : public uiDialog
{ mODTextTranslationClass(uiLatLong2CoordDlg);
public:
			uiLatLong2CoordDlg(uiParent*,const LatLong2Coord&,
					   const SurveyInfo* si=0);
			~uiLatLong2CoordDlg();

    const LatLong2Coord& ll2C() const		{ return ll2c_; }

    static bool		ensureLatLongDefined(uiParent*,SurveyInfo* si=0);

protected:

    LatLong2Coord&	ll2c_;
    const SurveyInfo*	si_;

    uiGenInput*		coordfld_;
    uiLatLongInp*	latlngfld_;

    bool		getLL2C();
    void		transfFile(CallBacker*);
    bool		acceptOK(CallBacker*) override;

};
