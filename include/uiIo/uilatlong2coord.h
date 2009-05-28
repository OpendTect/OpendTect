#ifndef uilatlong2coord_h
#define uilatlong2coord_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uilatlong2coord.h,v 1.3 2009-05-28 11:53:09 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class SurveyInfo;
class LatLong2Coord;
class uiGenInput;
class uiLatLongInp;


mClass uiLatLong2CoordDlg : public uiDialog
{
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

    bool		acceptOK(CallBacker*);
};


#endif
