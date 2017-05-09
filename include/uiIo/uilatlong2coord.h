#ifndef uilatlong2coord_h
#define uilatlong2coord_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "coordsystem.h"

class SurveyInfo;
class LatLong2Coord;
class uiGenInput;
class uiLatLongInp;
class uiLabel;
class uiCheckBox;


namespace Coords
{

mExpClass(uiIo) uiPositionSystem : public uiGroup
{
public:
    mDefineFactory1ParamInClass(uiPositionSystem,uiParent*,factory);

    virtual bool		initFields(const PositionSystem*)= 0;

    RefMan<PositionSystem>	outputSystem() { return outputsystem_; }
				//!<After AcceptOK();

    virtual HelpKey		helpKey() const { return helpkey_; }

    void			setSurveyInfo( const SurveyInfo* si )
				{ si_ = si; }
    virtual bool		acceptOK()	{ return false; }

protected:
				uiPositionSystem(uiParent*);
    RefMan<PositionSystem>	outputsystem_;
    HelpKey			helpkey_;
    const SurveyInfo*		si_;
};


mExpClass(uiIo) uiPositionSystemSel : public uiGroup
{ mODTextTranslationClass(uiCoordinateSystemSel);
public:
				uiPositionSystemSel(uiParent*,
						bool onlyorthogonal,
						const Coords::PositionSystem*);
				~uiPositionSystemSel();
    RefMan<PositionSystem>	outputSystem() { return outputsystem_; }
				//!<After AcceptOK();
    bool			acceptOK();

private:

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

    uiCheckBox*		xyinftfld_;

    bool		acceptOK();

};


mExpClass(uiIo) uiAnchorBasedXYSystem : public uiPositionSystem
{ mODTextTranslationClass(uiAnchorBasedXYSystem);
public:
    mDefaultFactoryInstantiation1Param( uiPositionSystem, uiAnchorBasedXYSystem,
			       uiParent*, AnchorBasedXY::sFactoryKeyword(),
			       AnchorBasedXY::sFactoryDisplayName() );

			uiAnchorBasedXYSystem(uiParent*);


    virtual bool	initFields(const PositionSystem*);

protected:

    uiGenInput*		coordfld_;
    uiLatLongInp*	latlngfld_;

    uiCheckBox*		xyinftfld_;

//    void		transfFile(CallBacker*);
    bool		acceptOK();

};


mExpClass(uiIo) uiCoordSystemDlg : public uiDialog
{ mODTextTranslationClass(uiLatLong2CoordDlg);
public:
			uiCoordSystemDlg(uiParent*,const PositionSystem*);
			~uiCoordSystemDlg();

    RefMan<PositionSystem> getCoordSystem();

    static bool		ensureLatLongDefined(uiParent*,SurveyInfo* si=0);

protected:

    uiPositionSystemSel* coordsysselfld_;

    void		transfFile(CallBacker*);
    bool		acceptOK(CallBacker*);

};

} // Namespace Coords

mExpClass(uiIo) uiLatLong2CoordDlg : public uiDialog
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


#endif
