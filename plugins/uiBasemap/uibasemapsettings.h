#ifndef uibasemapsettings_h
#define uibasemapsettings_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		April 2015
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uibasemapmod.h"

#include "draw.h"
#include "uidialog.h"

class Settings;
class uiBasemapView;
class uiColorInput;
class uiSelLineStyle;

mExpClass(uiBasemap) uiBasemapSettingsDlg : public uiDialog
{ mODTextTranslationClass(uiBasemapSettingsDlg)
public:
			uiBasemapSettingsDlg(uiParent*,uiBasemapView&);
			~uiBasemapSettingsDlg();

protected:
    void		changeSurvBoudColCB(CallBacker*);
    void		changeBgColorCB(CallBacker*);
    virtual bool	acceptOK(CallBacker*);
    virtual bool	rejectOK(CallBacker*);

    uiBasemapView&	basemapvw_;
    Settings&		setts_;
    uiSelLineStyle*	lsfld_;
    uiColorInput*	colbgfld_;

private:
    Color		originalbgcol_;
    LineStyle		originallst_;
};

#endif

