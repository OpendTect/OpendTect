#ifndef uisurvey_h
#define uisurvey_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "survinfo.h"
#include "uichecklist.h"
#include "uiiomod.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "bufstring.h"
#include "bufstringset.h"

class uiComboBox;
class uiLabel;
class uiGraphicsScene;
class uiGraphicsView;
class uiListBox;
class uiTextEdit;
class uiSurveyMap;
class uiPushButton;
class uiToolButton;
class uiSurveySelect;
class uiSurvInfoProvider;


/*!\brief The main survey selection dialog */

mExpClass(uiIo) uiSurvey : public uiDialog
{

public:
			uiSurvey(uiParent*);
			~uiSurvey();

    static void		getSurveyList(BufferStringSet&,const char* dataroot=0);

    static bool		survTypeOKForUser(bool is2d);
    			//!< checks whether given type has support
    			//!< returns whether user wants to continue

    /*!\brief 'Menu' item on window. First is always 'X,Y <-> I/C' */
    struct Util
    {
			Util( const char* pixmap, const char* tooltip,
				const CallBack& cb )
			    : cb_(cb)
			    , pixmap_(pixmap)
			    , tooltip_(tooltip)		{}

	CallBack	cb_;
	BufferString	pixmap_;
	BufferString	tooltip_;
    };
    static void		add(const Util&);

    SurveyInfo*		curSurvInfo()		{ return survinfo_; }
    const SurveyInfo*	curSurvInfo() const	{ return survinfo_; }

protected:

    SurveyInfo*		survinfo_;
    BufferStringSet	dirlist_;
    BufferString	initialdatadir_;
    BufferString	initialsurvey_;
    uiSurveyMap*	survmap_;
    IOPar*		impiop_;
    uiSurvInfoProvider*	impsip_;

    uiListBox*		listbox_;
    uiPushButton*	newbut_;
    uiPushButton*	editbut_;
    uiPushButton*	rmbut_;
    uiPushButton*	datarootbut_;
    uiPushButton*	copybut_;
    uiToolButton*	exportbut_;
    uiToolButton*	importbut_;
    ObjectSet<uiToolButton> utilbuts_;
    uiLabel*		inllbl_;
    uiLabel*		crllbl_; 
    uiLabel*		zlbl_;
    uiLabel*		binlbl_;
    uiLabel*		arealbl_;
    uiLabel*		typelbl_;
    uiTextEdit*		notes_;
    bool		initialsurveyparchanged_;

    bool		acceptOK(CallBacker*);  
    bool		rejectOK(CallBacker*);  
    void		newButPushed(CallBacker*);
    void		rmButPushed(CallBacker*);
    void		editButPushed(CallBacker*);
    void		copyButPushed(CallBacker*);
    void		importButPushed(CallBacker*);
    void		exportButPushed(CallBacker*);
    void		dataRootPushed(CallBacker*);
    void		utilButPush(CallBacker*);
    void		updateSvyList();
    bool		updateSvyFile();
    bool 		getSurvInfo();
    void		delSurvInfo();
    void 		updateInfo(CallBacker*);
    bool		survInfoDialog(bool isnew);
    void		selChange(CallBacker*);
    void		mkDirList();
    void		mkInfo();
    void		writeComments();
    bool		writeSurveyName(const char*);
    void		updateDataRootInSettings();

};


/*!\brief The new survey selection dialog */

mExpClass(uiIo)	uiSurveyNewDlg : public uiDialog
{

public:
    			uiSurveyNewDlg(uiParent*,SurveyInfo&);

    bool		isOK();
    bool		acceptOK(CallBacker*);

protected:

    SurveyInfo&		survinfo_;
    uiGenInput*		survnmfld_;
    uiCheckList*	pol2dfld_;
    uiComboBox*		sipfld_;
    uiCheckList*	zdomainfld_;
    uiGroup*		zunitgrp_;
    uiCheckList*	zunitfld_;
    ObjectSet<uiSurvInfoProvider>& sips_;

    BufferString	survName() const { return survnmfld_->text(); }
    SurveyInfo::Pol2D	pol2D() const;
    bool		has3D() const { return pol2dfld_->isChecked(0); }
    bool		has2D() const { return pol2dfld_->isChecked(1); }
    BufferString	sipName() const;
    bool		isTime() const { return zdomainfld_->isChecked(0); }
    bool		isInFeet() const { return zunitfld_->isChecked(1); }

    void		setSip(bool for2donly);
    void		pol2dChg(CallBacker*);
    void		zdomainChg(CallBacker*);

};

/*!\brief The copy survey selection dialog */

mExpClass(uiIo) uiSurveyGetCopyDir : public uiDialog
{

public:
			uiSurveyGetCopyDir(uiParent*,const char* cursurv);

    void		inpSel(CallBacker*);
    bool		acceptOK(CallBacker*);

    BufferString	fname_;
    BufferString	newdirnm_;
    uiSurveySelect*	inpsurveyfld_;
    uiSurveySelect*	newsurveyfld_;

};

#endif

