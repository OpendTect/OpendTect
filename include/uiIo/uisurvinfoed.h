#ifndef uisurvinfoed_h
#define uisurvinfoed_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvinfoed.h,v 1.7 2002-01-04 00:08:46 bert Exp $
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
    FileNameString	rootdir;
    BufferString	survnm;
    BufferString	dirnm;

    uiGenInput*		survnmfld;
    uiGenInput*		dirnmfld;
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
    uiGroup*		coordgrp;
    uiGroup*		trgrp;
    uiLabel*		labelcs;
    uiLabel*		labeltr;
    uiCheckBox*		overrule;
    uiPushButton*	applybut;

    bool		dirnmch_;
    void		setValues();
    bool		setRanges();
    bool		setCoords();
    bool		setRelation();
    bool		appButPushed();
    void		wsbutPush( CallBacker* );
    void		chgSetMode( CallBacker* );
    bool		acceptOK(CallBacker*);
    void		doFinalise();
    void		setInl1Fld( CallBacker* );
};

#endif
