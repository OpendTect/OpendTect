#ifndef uilatlong2coord_h
#define uilatlong2coord_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
class SurveyInfo;
class LatLong2Coord;
class uiGenInput;
class uiLatLongInp;


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
