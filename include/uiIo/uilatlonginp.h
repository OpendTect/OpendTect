#ifndef uilatlonginp_h
#define uilatlonginp_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          May 2009
 RCS:           $Id: uilatlonginp.h,v 1.1 2009-05-28 11:49:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class LatLong;
class uiLineEdit;
class uiRadioButton;
class uiLatLongDMSInp;


mClass uiLatLongInp : public uiGroup
{
public:

    			uiLatLongInp(uiParent*);

    void		get(LatLong&) const;
    void		set(const LatLong&);

protected:

    uiRadioButton*	isdecbut_;
    uiLineEdit*		latdecfld_;
    uiLineEdit*		lngdecfld_;
    uiLatLongDMSInp*	latdmsfld_;
    uiLatLongDMSInp*	lngdmsfld_;

    void		get(LatLong&,bool) const;
    void		set(const LatLong&,int);

    void		typSel(CallBacker*);
};


#endif
