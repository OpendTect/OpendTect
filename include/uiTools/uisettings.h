#ifndef uisettings_h
#define uisettings_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2004
 RCS:		$Id: uisettings.h,v 1.7 2005-11-02 16:47:06 cvsarend Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class Settings;
class uiGenInput;


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

protected:

    Settings&		setts;
    int			iconsz;
    bool		isvert;
    bool		isontop;

    uiGenInput*		iconszfld;
    uiGenInput*		colbarhvfld;
    uiGenInput*		colbarontopfld;

    bool		acceptOK(CallBacker*);

};

#endif
