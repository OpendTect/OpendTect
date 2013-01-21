#ifndef uisip_h
#define uisip_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "ranges.h"

class Coord;
class CubeSampling;
class SurveyInfo;
class uiDialog;


/*\brief Interface for survey info provider

  The survey setup can be delivered by scanning files (SEG-Y) or by querying
  data stores like SeisWorks and GeoFrame. Another idea not implemented may be
  to read a simple ascii file. In any case, this is the interface. The
  implementation should be added to the factory using 
  uiSurveyInfoEditor::addInfoProvider.

  In some cases, the provider can also allow actual data import. After
  setting up the survey, the user can be automatically put into an import
  facility. If that is the case, you can return an IOPar in getImportPars()
  that, when the time comes, will be used to call startImport().
 
 */

mExpClass(uiIo) uiSurvInfoProvider
{
public:

    virtual const char*		usrText() const		= 0;
    virtual uiDialog*		dialog(uiParent*)	= 0;
    virtual bool		getInfo(uiDialog*,CubeSampling&,
					Coord crd[3])	= 0;

    virtual const char*		scanFile() const	{ return 0; }

    enum TDInfo			{ Uknown, Time, Depth, DepthFeet };
    virtual TDInfo		tdInfo() const		{ return Uknown; }
    virtual bool		xyInFeet() const	{ return false; }

    virtual IOPar*		getImportPars() const	{ return 0; }
    virtual void		startImport(uiParent*,const IOPar&) {}
    virtual const char*		importAskQuestion() const
				{ return "Proceed to import?"; }

};


#endif

