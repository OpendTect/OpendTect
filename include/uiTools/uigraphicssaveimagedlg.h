#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          February 2009
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uisaveimagedlg.h"

class uiGraphicsScene;

mExpClass(uiTools) uiGraphicsSaveImageDlg : public uiSaveImageDlg
{ mODTextTranslationClass(uiGraphicsSaveImageDlg);
public:

			uiGraphicsSaveImageDlg(uiParent*,uiGraphicsScene*);

protected:

    uiGraphicsScene*	scene_;

    bool		supportPrintFormats() const	{ return true; }
    void		writeToSettings();
    void		setAspectRatio(CallBacker*);
    void		setFldVals(CallBacker*);
    bool		acceptOK();

};
