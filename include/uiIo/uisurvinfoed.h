#ifndef uisurvinfoed_h
#define uisurvinfoed_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvinfoed.h,v 1.14 2004-01-19 15:56:37 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"
#include "ranges.h"
class uiCheckBox;
class uiGenInput;
class uiPushButton;
class uiRadioButton;
class BinIDSampler;
class SurveyInfo;
class uiLabel;
class Coord;


/*\brief Interface for survey info provider - typically from workstations */

class uiSurvInfoProvider
{
public:

    virtual const char*		name() const		= 0;
    virtual uiDialog*		dialog(uiParent*)	= 0;
    virtual bool		getInfo(uiDialog*,BinIDSampler&,
	    				StepInterval<double>&,
					Coord crd[3])	= 0;
};


/*\brief The survey info editor */

class uiSurveyInfoEditor : public uiDialog
{

public:
			uiSurveyInfoEditor(uiParent*,SurveyInfo*);

    bool		dirnmChanged() const		{ return dirnmch_; }
    const char*		dirName();
    Notifier<uiSurveyInfoEditor> survparchanged;

    static int		addInfoProvider(uiSurvInfoProvider*);
    			
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
    uiRadioButton*	timefld;
    uiRadioButton*	meterfld;
    uiRadioButton*	feetfld;
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
    ObjectSet<CallBacker> sipbuts;

    bool		dirnmch_;
    void		setValues();
    bool		setRanges();
    bool		setCoords();
    bool		setRelation();
    bool		appButPushed();
    bool		acceptOK(CallBacker*);
    void		sipbutPush(CallBacker*);
    void		doFinalise(CallBacker*);
    void		setInl1Fld(CallBacker*);
    void		chgSetMode(CallBacker*);
    void		pathbutPush(CallBacker*);
    void		unitPush(CallBacker*);
    void		updStatusBar(const char*);

};

#endif
