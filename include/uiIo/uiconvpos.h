#ifndef uiconvpos_h
#define uiconvpos_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uiconvpos.h,v 1.9 2009/07/22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiGenInput;
class uiFileInput;
class SurveyInfo;


mClass uiConvertPos : public uiDialog
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
