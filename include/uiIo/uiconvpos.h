#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"
class uiGenInput;
class uiFileInput;
class SurveyInfo;


mExpClass(uiIo) uiConvertPos : public uiDialog
{ mODTextTranslationClass(uiConvertPos);

public:
                        uiConvertPos(uiParent*, const SurveyInfo&,
				     bool modal=true);

private:

    const SurveyInfo&	survinfo;

    uiGenInput*		ismanfld;
    uiGroup*		mangrp;
    uiGroup*		filegrp;
    uiGenInput*		inlfld;
    uiGenInput*		crlfld;
    uiGenInput*		xfld;
    uiGenInput*		yfld;
    uiFileInput*	inpfilefld;
    uiFileInput*	outfilefld;
    uiGenInput*		isxy2bidfld;

    void		selChg(CallBacker*);
    void		getCoord(CallBacker*);
    void		getBinID(CallBacker*);
    void		convFile(CallBacker*);
};
