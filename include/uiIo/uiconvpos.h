#ifndef uiconvpos_h
#define uiconvpos_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uiconvpos.h,v 1.10 2012-08-03 13:00:59 cvskris Exp $
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
class uiGenInput;
class uiFileInput;
class SurveyInfo;


mClass(uiIo) uiConvertPos : public uiDialog
{

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

#endif

