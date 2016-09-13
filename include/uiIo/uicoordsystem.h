#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2008
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "coordsystem.h"
#include "factory.h"
#include "uidlggroup.h"

class LatLong2Coord;
class SurveyInfo;
class uiGenInput;
class uiLatLongInp;
class uiLabel;
class uiCheckBox;

namespace Coords
{


mExpClass(uiIo) uiPositionSystem : public uiDlgGroup
{
public:
    mDefineFactory1ParamInClass(uiPositionSystem,uiParent*,factory);

    virtual bool		initFields(const PositionSystem*)= 0;

    RefMan<PositionSystem>	outputSystem() { return outputsystem_; }
				//!<After AcceptOK();

    virtual HelpKey		helpKey() const { return helpkey_; }

    void			setSurveyInfo( const SurveyInfo* si )
				{ si_ = si; }

protected:
				uiPositionSystem(uiParent*,const uiString&);
    RefMan<PositionSystem>	outputsystem_;
    HelpKey			helpkey_;
    const SurveyInfo*		si_;
};


mExpClass(uiIo) uiPositionSystemSel : public uiDlgGroup
{ mODTextTranslationClass(uiCoordinateSystemSel);
public:
				uiPositionSystemSel(uiParent*,
						bool onlyorthogonal,
						const SurveyInfo*,
						const Coords::PositionSystem*);
				~uiPositionSystemSel();
    RefMan<PositionSystem>	outputSystem() { return outputsystem_; }
				//!<After AcceptOK();
private:

    void			convertFileCB(CallBacker*);
    bool			acceptOK();
    void			systemChangedCB(CallBacker*);
    uiGenInput*			coordsystemsel_;
    uiLabel*			coordsystemdesc_;
    ObjectSet<uiPositionSystem> coordsystemsuis_;
    ManagedObjectSet<IOPar>	coordsystempars_;

    RefMan<PositionSystem>	outputsystem_;
};



mExpClass(uiIo) uiUnlocatedXYSystem : public uiPositionSystem
{ mODTextTranslationClass(uiUnlocatedXYSystem);
public:
    mDefaultFactoryInstantiation1Param( uiPositionSystem, uiUnlocatedXYSystem,
			       uiParent*, UnlocatedXY::sFactoryKeyword(),
			       UnlocatedXY::sFactoryDisplayName() );

			uiUnlocatedXYSystem(uiParent*);


    virtual bool	initFields(const PositionSystem*);

protected:

    uiGenInput*		coordfld_;
    uiLatLongInp*	latlngfld_;

    uiCheckBox*		xyinftfld_;

//    void		transfFile(CallBacker*);
    bool		acceptOK();

};

} //Namespace
