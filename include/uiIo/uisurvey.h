#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "bufstring.h"
#include "survinfo.h"

class BufferStringSet;
class SurveyInfo;
class uiButton;
class uiCheckList;
class uiDataRootSel;
class uiGenInput;
class uiLabel;
class uiLineEdit;
class uiListBox;
class uiSurveyMap;
class uiSurvInfoProvider;
class uiTextEdit;


/*!\brief The main survey selection dialog */

mExpClass(uiIo) uiSurvey : public uiDialog
{ mODTextTranslationClass(uiSurvey);

public:
			uiSurvey(uiParent*);
			~uiSurvey();

    static void		getSurveyList(BufferStringSet&,const char* dataroot=0,
				      const char* excludenm=0);

    static bool		survTypeOKForUser(bool is2d);
			//!< checks whether given type has support
			//!< returns whether user wants to continue

    static bool		ensureValidDataRoot();

    /*!\brief 'Menu' item on window. First is always 'X,Y <-> I/C' */
    struct Util
    {
			Util( const char* pixmap, const uiString& tooltip,
				const CallBack& cb )
			    : cb_(cb)
			    , pixmap_(pixmap)
			    , tooltip_(tooltip)		{}

	CallBack	cb_;
	BufferString	pixmap_;
	uiString	tooltip_;
    };
    static void		add(const Util&);

    SurveyInfo*		curSurvInfo()		{ return cursurvinfo_; }
    const SurveyInfo*	curSurvInfo() const	{ return cursurvinfo_; }

    const char*		selectedSurveyName() const;
    bool		freshSurveySelected() const
			{ return freshsurveyselected_; }
    bool		hasSurveys() const;
    bool		currentSurvRemoved() const { return cursurvremoved_; }

protected:

    SurveyInfo*		cursurvinfo_;
    const BufferString	orgdataroot_;
    BufferString	dataroot_;
    BufferString	initialsurveyname_;
    uiSurveyMap*	survmap_;
    IOPar*		impiop_;
    uiSurvInfoProvider*	impsip_;
    BufferStringSet	surveynames_;
    BufferStringSet	surveydirs_;

    uiDataRootSel*	datarootsel_;
    uiListBox*		dirfld_;
    uiButton*		editbut_;
    uiButton*		rmbut_;
    ObjectSet<uiButton>	utilbuts_;
    uiTextEdit*		infofld_;
    uiTextEdit*		notesfld_;
    uiTextEdit*		logfld_;
    IOPar		infopars_;

    bool		parschanged_; //!< of initial survey only
    bool		cursurvremoved_;
    bool		freshsurveyselected_;

    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);
    void		newButPushed(CallBacker*);
    void		rmButPushed(CallBacker*);
    void		editButPushed(CallBacker*);
    void		copyButPushed(CallBacker*);
    void		importButPushed(CallBacker*);
    void		exportButPushed(CallBacker*);
    void		dataRootChgCB(CallBacker*);
    void		odSettsButPush(CallBacker*);
    void		utilButPush(CallBacker*);
    void		selChange(CallBacker*);
    void		updateInfo( CallBacker* )	{ putToScreen(); }

    void		updateSurveyNames();
    void		readSurvInfoFromFile();
    void		setCurrentSurvInfo(SurveyInfo*,bool updscreen=true);
    void		updateSurvList();
    void		putToScreen();
    bool		checkSurveyName();
    bool		writeSurvInfoFileIfCommentChanged();
    bool		rootDirWritable() const;
    bool		doSurvInfoDialog(bool isnew);
    void		updateDataRootInSettings();
    void		rollbackNewSurvey(const uiString&);
    void		copyInfoToClipboard();

private:
    void		fillLeftGroup(uiGroup*);
    void		fillRightGroup(uiGroup*);
};


//--- uiStartNewSurveySetup


mExpClass(uiIo) uiStartNewSurveySetup : public uiDialog
{ mODTextTranslationClass(uiStartNewSurveySetup);

public:
			uiStartNewSurveySetup(uiParent*,const char*,
					      SurveyInfo&);

    void		setSurveyNameFld(BufferString,bool);
    bool		isOK();
    bool		acceptOK(CallBacker*);

    ObjectSet<uiSurvInfoProvider> sips_;
    int			sipidx_;

private:

    const BufferString	dataroot_;
    SurveyInfo&		survinfo_;
    uiGenInput*		survnmfld_;
    uiGenInput*		zistimefld_;
    uiGenInput*		zinfeetfld_;
    uiCheckList*	pol2dfld_;
    uiListBox*		sipfld_;

    BufferString	sipName() const;
    BufferString	survName() const;
    bool		has3D() const;
    bool		has2D() const;
    bool		hasWells() const;
    bool		isTime() const;
    bool		isInFeet() const;
    void		fillSipsFld(bool have2d,bool have3d,bool havewells);
    SurveyInfo::Pol2D	pol2D() const;
    void		pol2dChg(CallBacker*);
    void		zdomainChg(CallBacker*);

};
