#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		February 2009
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
  
    void		getSupportedFormats(const char** imagefrmt,
					    const char** frmtdesc,
					    BufferString& filters) override;
    void		writeToSettings() override;
    void		setAspectRatio(CallBacker*);
    void		setFldVals(CallBacker*) override;
    bool		acceptOK(CallBacker*) override;
};

