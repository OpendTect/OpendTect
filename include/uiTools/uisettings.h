#ifndef uisettings_h
#define uisettings_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2004
 RCS:		$Id: uisettings.h,v 1.14 2009-04-01 04:39:05 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

class Settings;
class uiGenInput;
struct LooknFeelSettings;


mClass uiSettings : public uiDialog
{
public:
			uiSettings(uiParent*,const char* titl,
				   const char* settskey=0);
    virtual		~uiSettings();

protected:

    Settings&		setts_;

    uiGenInput*		keyfld_;
    uiGenInput*		valfld_;

    void		selPush(CallBacker*);
    bool		acceptOK(CallBacker*);

};


mClass uiLooknFeelSettings : public uiDialog
{
public:
			uiLooknFeelSettings(uiParent*,const char* titl);
    virtual		~uiLooknFeelSettings();

    bool		isChanged() const	{ return changed_; }

protected:

    Settings&		setts_;
    LooknFeelSettings&	lfsetts_;
    bool		changed_;

    uiGenInput*		iconszfld_;
    uiGenInput*		colbarhvfld_;
    uiGenInput*		colbarontopfld_;
    uiGenInput*		showwheelsfld_;
    uiGenInput*		showprogressfld_;
    uiGenInput*		useshadingfld_;
    uiGenInput*		volrenshadingfld_;

    void		updateSettings(bool oldval,bool newval,const char* key);

    void		shadingChange(CallBacker*);
    bool		acceptOK(CallBacker*);
};

#endif
