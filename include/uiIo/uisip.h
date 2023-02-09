#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "coord.h"

class LatLong;
class SurveyInfo;
class TrcKeyZSampling;
class uiDialog;


/*\brief Interface for survey info provider

  The survey setup can be delivered by scanning files (SEG-Y) or by querying
  data stores like SeisWorks, Petrel, .... Another idea not implemented may be
  to read a simple ascii file. In any case, this is the interface. The
  implementation should be added to the factory using
  uiSurveyInfoEditor::addInfoProvider.

  The logic is:

  * User gets list showing - if isAvailable() - your usrText() and icon
  * User fills in your dialog
  * On OK, getInfo is called
  * If getInfo returns true, then you can stuff your own things in the
    new SurveyInfo's IOPar if you define fillPar().

  In some cases, the provider can also allow actual data import. After
  setting up the survey, the user can be automatically put into an import
  facility. If that is the case, you have to return an IOPar by getImportPars()
  that, when the time comes, will be used to call startImport().

  This class delivers everything in SI units.

 */

mExpClass(uiIo) uiSurvInfoProvider
{
public:

    virtual			~uiSurvInfoProvider();

    virtual const char*		usrText() const		= 0;
    virtual uiDialog*		dialog(uiParent*)	= 0;
    virtual uiDialog*		launchSurveyImportDlg(uiParent*)
						     { return nullptr;}
    virtual bool		hasSurveyImportDlg() { return false; }
    virtual bool		getInfo(uiDialog*,TrcKeyZSampling&,
					Coord crd[3])	= 0;
    virtual bool		getLatLongAnchor(Coord&,LatLong&)
							{ return false; }
    virtual bool		getSRD(float&)		{ return false; }
				//!< return value must be in SI units (meters)

    virtual void		fillPar(IOPar&) const		{}
    virtual void		fillLogPars(IOPar&) const;
    virtual bool		isAvailable() const	{ return true; }

    enum TDInfo			{ Uknown, Time, Depth, DepthFeet };
    virtual TDInfo		tdInfo() const		{ return Uknown; }
    virtual bool		xyInFeet() const	{ return false; }
    virtual const char*		iconName() const	{ return nullptr; }

    virtual IOPar*		getImportPars() const	{ return nullptr; }
    virtual void		startImport(uiParent*,const IOPar&) {}
    virtual const char*		importAskQuestion() const
				{ return "Proceed to import?"; }

    virtual IOPar*		getCoordSystemPars() const  { return nullptr; }

    static bool			getRanges(TrcKeyZSampling&,Coord[3],
					  Coord crd1,Coord crd2,
					  double gridspacing);
				//!<Calculates sampling and 3 corner coordinates

protected:
				uiSurvInfoProvider();
};
