#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
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

    virtual bool	initFields(const CoordSystem*);

protected:

    uiGenInput*		xyunitfld_;

    bool		acceptOK();

};


mExpClass(uiTools) uiAnchorBasedXYSystem : public uiCoordSystem
{ mODTextTranslationClass(uiAnchorBasedXYSystem);
public:
    mDefaultFactoryInstantiation1Param( uiCoordSystem, uiAnchorBasedXYSystem,
			       uiParent*, AnchorBasedXY::sFactoryKeyword(),
			       AnchorBasedXY::sFactoryDisplayName() );

			uiAnchorBasedXYSystem(uiParent*);

    virtual bool	initFields(const CoordSystem*);

protected:

    uiGenInput*		coordfld_;
    uiLatLongInp*	latlngfld_;
    uiGenInput*		xyunitfld_;

    bool		acceptOK();

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
    bool		acceptOK(CallBacker*);

};


