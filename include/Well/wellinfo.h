#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2016
________________________________________________________________________

-*/

#include "wellcommon.h"
#include "namedmonitorable.h"
#include "enums.h"
#include "position.h"
#include "uistring.h"


namespace Well
{


/*!\brief Information about a certain well. */

mExpClass(Well) Info : public NamedMonitorable
{ mODTextTranslationClass(Well::Info)
public:

    typedef float	VelType;
    typedef float	ElevType;
    enum WellType	{ None, Oil, Gas, OilGas, Dry, PluggedOil,
			  PluggedGas, PluggedOilGas, PermLoc, CancLoc,
			  InjectDispose };
			mDeclareEnumUtils(WellType);

    enum DepthType	{ MD, TVD, TVDSS };
			mDeclareEnumUtils(DepthType);

			Info(const char* nm=0);
			~Info();
			mDeclMonitorableAssignment(Info);
			mDeclInstanceCreatedNotifierAccess(Info);

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    mImplSimpleMonitoredGetSet(inline,UWI,setUWI,
			BufferString,uwid_,cUWIDChange())
    mImplSimpleMonitoredGetSet(inline,wellOperator,setWellOperator,
			BufferString,oper_,cInfoChange())
    mImplSimpleMonitoredGetSet(inline,getState,setState,
			BufferString,state_,cInfoChange())
    mImplSimpleMonitoredGetSet(inline,getCounty,setCounty,
			BufferString,county_,cInfoChange())
    mImplSimpleMonitoredGetSet(inline,wellType,setWellType,
			WellType,welltype_,cTypeChange())
    mImplSimpleMonitoredGetSet(inline,surfaceCoord,setSurfaceCoord,
			Coord,surfacecoord_,cPosChg())
    mImplSimpleMonitoredGetSet(inline,replacementVelocity,
			setReplacementVelocity,VelType,replvel_,cVelChg())
    mImplSimpleMonitoredGetSet(inline,groundElevation,setGroundElevation,
			ElevType,groundelev_,cElevChg())
    mImplSimpleMonitoredGetSet(inline,dataSource,setDataSource,
			BufferString,source_,cSourceChg());
				//!< .e.g. the file name of the .well file

    static const char*	sKeyUwid();
    static const char*	sKeyOper();
    static const char*	sKeyState();
    static const char*	sKeyCounty();
    static const char*	sKeyCoord();
    static const char*	sKeyKBElev();
    static const char*	sKeyTD();
    static const char*	sKeyTVD();
    static const char*	sKeyTVDSS();
    static const char*	sKeyMD();
    static const char*	sKeyReplVel();
    static const char*	sKeyGroundElev();
    static const char*	sKeyWellName();
    static const char*	sKeyWellType();

    static uiString	sUwid();
    static uiString	sOper();
    static uiString	sState();
    static uiString	sCounty();
    static uiString	sCoord();
    static uiString	sKBElev();
    static uiString	sTD();
    static uiString	sReplVel();
    static uiString	sGroundElev();

    static ChangeType	cUWIDChange()	{ return 2; }
    static ChangeType	cInfoChange()	{ return 3; }
    static ChangeType	cTypeChange()	{ return 4; }
    static ChangeType	cPosChg()	{ return 5; }
    static ChangeType	cVelChg()	{ return 6; }
    static ChangeType	cElevChg()	{ return 7; }
    static ChangeType	cSourceChg()	{ return 8; };

protected:

    BufferString	uwid_;
    BufferString	oper_;
    BufferString	state_;
    BufferString	county_;
    WellType		welltype_;
    Coord		surfacecoord_;
    VelType		replvel_;
    ElevType		groundelev_;
    BufferString	source_;

};


} // namespace Well
