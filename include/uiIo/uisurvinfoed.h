#ifndef uisurvinfoed_h
#define uisurvinfoed_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvinfoed.h,v 1.10 2002-10-08 11:50:22 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"
class uiCheckBox;
class uiGenInput;
class uiPushButton;
class SurveyInfo;
class uiLabel;


class uiSurveyInfoEditor : public uiDialog
{

public:
			uiSurveyInfoEditor(uiParent*,SurveyInfo*,
					   const CallBack&);
    bool		dirnmChanged() const		{ return dirnmch_; }
    const char*		dirName();
    Notifier<uiSurveyInfoEditor> survparchanged;
    SurveyInfo*		getSurvInfo()			{ return survinfo; }

protected:

    SurveyInfo*		survinfo;
    UserIDString	orgdirname;
    const FileNameString rootdir;

    uiGenInput*		survnmfld;
    uiGenInput*		dirnmfld;
    uiGenInput*		pathfld;
    uiGenInput*		inlfld;
    uiGenInput*		crlfld;
    uiGenInput*		zfld;
    uiGenInput*		x0fld;
    uiGenInput*		xinlfld;
    uiGenInput*		xcrlfld;
    uiGenInput*		y0fld;
    uiGenInput*		yinlfld;
    uiGenInput*		ycrlfld;
    uiGenInput*		ic0fld;
    uiGenInput*		ic1fld;
    uiGenInput*		ic2fld;
    uiGenInput*		xy0fld;
    uiGenInput*		xy1fld;
    uiGenInput*		xy2fld;
    uiGenInput*		coordset;
    uiGroup*		crdgrp;
    uiGroup*		trgrp;
    uiCheckBox*		overrule;
    uiPushButton*	applybut;

    bool		dirnmch_;
    void		setValues();
    bool		setRanges();
    bool		setCoords();
    bool		setRelation();
    bool		appButPushed();
    bool		acceptOK(CallBacker*);
    void		wsbutPush(CallBacker*);
    void		doFinalise(CallBacker*);
    void		setInl1Fld(CallBacker*);
    void		chgSetMode(CallBacker*);
    void		pathbutPush(CallBacker*);
    void		updStatusBar(const char*);
};

#endif
