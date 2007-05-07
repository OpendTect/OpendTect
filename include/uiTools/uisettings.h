#ifndef uisettings_h
#define uisettings_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2004
 RCS:		$Id: uisettings.h,v 1.10 2007-05-07 05:50:30 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class Settings;
class uiGenInput;
class LooknFeelSettings;


class uiSettings : public uiDialog
{
public:
			uiSettings(uiParent*,const char* titl,
				   const char* settskey=0);
    virtual		~uiSettings();

protected:

    Settings&		setts;

    uiGenInput*		keyfld;
    uiGenInput*		valfld;

    void		selPush(CallBacker*);
    bool		acceptOK(CallBacker*);

};


class uiLooknFeelSettings : public uiDialog
{
public:
			uiLooknFeelSettings(uiParent*,const char* titl);
    virtual		~uiLooknFeelSettings();

    bool		isChanged() const	{ return changed; }

protected:

    Settings&		setts;
    LooknFeelSettings&	lfsetts;
    bool		changed;

    uiGenInput*		iconszfld;
    uiGenInput*		colbarhvfld;
    uiGenInput*		colbarontopfld;
    uiGenInput*		useshadingfld;

    void		ctOrientChange(CallBacker*);

    bool		acceptOK(CallBacker*);

};

#endif
