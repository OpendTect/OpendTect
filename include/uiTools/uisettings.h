#ifndef uisettings_h
#define uisettings_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2004
 RCS:		$Id: uisettings.h,v 1.6 2004-12-23 16:47:57 bert Exp $
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

#endif
