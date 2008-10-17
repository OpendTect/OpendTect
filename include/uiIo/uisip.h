#ifndef uisip_h
#define uisip_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2008
 RCS:           $Id: uisip.h,v 1.1 2008-10-17 13:06:53 cvsbert Exp $
________________________________________________________________________

-*/

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
 
 */

class uiSurvInfoProvider
{
public:

    virtual const char*		usrText() const		= 0;
    virtual uiDialog*		dialog(uiParent*)	= 0;
    virtual bool		getInfo(uiDialog*,CubeSampling&,
					Coord crd[3])	= 0;

    virtual const char*		scanFile() const	{ return 0; }

    enum TDInfo			{ Uknown, Time, Depth, DepthFeet };
    virtual TDInfo		tdInfo() const		{ return Uknown; }
};


#endif
