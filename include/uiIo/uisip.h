#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2008
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"
#include "uigroup.h"
#include "ranges.h"

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

  Note: the userText's original (untranslated) string is put int the .survey.

 */

mExpClass(uiIo) uiSurvInfoProvider
{ mODTextTranslationClass(uiSurvInfoProvider)
public:

    virtual			~uiSurvInfoProvider()	{}

    enum TDInfo			{ Time, Depth, DepthFeet };
    static TDInfo		getTDInfo(bool istime,bool zinft=false);

    virtual uiString		usrText() const		= 0;
    virtual uiDialog*		dialog(uiParent*,TDInfo) = 0;
    virtual bool		getInfo(uiDialog*,TrcKeyZSampling&,
					Coord crd[3])	= 0;
    virtual bool		getLatLongAnchor(Coord&,LatLong&)
							{ return false; }
    virtual bool		getSRD(float&)		{ return false; }
				//!< return value must be in SI units (meters)

    virtual void		fillPar(IOPar&) const	{}
    virtual bool		isAvailable() const	{ return true; }

    virtual TDInfo		tdInfo( bool& isknown ) const
				{ isknown = false; return Time; }
    virtual bool		xyInFeet() const	{ return false; }
    virtual const char*		iconName() const	{ return 0; }

    virtual IOPar*		getImportPars() const	{ return 0; }
    virtual IOPar*		getCoordSystemPars() const  { return 0; }
    virtual void		startImport(uiParent*,const IOPar&) {}
    virtual uiString		importAskQuestion() const
					{ return uiString::empty(); }

    bool			runDialog(uiParent*,TDInfo,SurveyInfo&,
					  bool defdpthinft,bool* havezinfo=0);

    static const char*		sKeySIPName()		{ return "SIP.Name"; }

    static uiSurvInfoProvider*	getByName(const char*);
    static uiSurvInfoProvider*	getByName(const uiString&);

};
