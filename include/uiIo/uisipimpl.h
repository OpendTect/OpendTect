#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2004
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uisip.h"

namespace Coords { class CoordSystem; }


mExpClass(uiIo) ui2DSurvInfoProvider : public uiSurvInfoProvider
{ mODTextTranslationClass(ui2DSurvInfoProvider)
public:
			ui2DSurvInfoProvider()
			    : xyft_(false)	{}
			~ui2DSurvInfoProvider()	{}

    virtual uiString	usrText() const		{ return tr("Set for 2D only");}
    virtual uiDialog*	dialog(uiParent*,TDInfo);
    virtual bool	getInfo(uiDialog*,TrcKeyZSampling&,Coord crd[3]);
    virtual const char*	iconName() const
					{ return "seismicline2dcollection"; }

    virtual bool	xyInFeet() const	{ return xyft_; }

protected:

    bool		xyft_;
};


mExpClass(uiIo) uiCopySurveySIP : public uiSurvInfoProvider
{ mODTextTranslationClass(uiCopySurveySIP)
public:
			uiCopySurveySIP()	{}
			~uiCopySurveySIP()	{}

    virtual uiString	usrText() const
			{ return tr("Copy from other survey"); }

    virtual uiDialog*	dialog(uiParent*,TDInfo);
    virtual bool	getInfo(uiDialog*,TrcKeyZSampling&,Coord crd[3]);
    virtual const char*	iconName() const    { return "copyobj"; }

    virtual TDInfo	tdInfo( bool& known ) const
			{ known = tdinfknown_; return tdinf_; }
    virtual bool	xyInFeet() const    { return xyinft_; }
    virtual IOPar*	getCoordSystemPars() const;

protected:

    TDInfo		tdinf_;
    bool		tdinfknown_;
    bool		xyinft_;
    RefMan<Coords::CoordSystem>	coordsystem_;

};


mExpClass(uiIo) uiSurveyFileSIP : public uiSurvInfoProvider
{ mODTextTranslationClass(uiSurveyFileSIP)
public:
			uiSurveyFileSIP();
			~uiSurveyFileSIP()	{}

    virtual uiString	usrText() const;
    virtual uiDialog*	dialog(uiParent*,TDInfo);
    virtual bool	getInfo(uiDialog*,TrcKeyZSampling&,Coord crd[3]);
    virtual const char*	iconName() const	{ return "od"; }

    virtual TDInfo	tdInfo( bool& known ) const
			{ known = tdinfknown_; return tdinf_; }
    virtual bool	xyInFeet() const	{ return xyinft_; }

    virtual IOPar*	getCoordSystemPars() const;

protected:

    TDInfo		tdinf_;
    bool		tdinfknown_;
    bool		xyinft_;
    RefMan<Coords::CoordSystem>	coordsystem_;

};
