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

class BufferStringSet;
class SurveyInfo;
class uiButton;
class uiListBox;
class uiTextEdit;
class uiComboBox;
class uiLineEdit;
class uiSurveyMap;
class uiDataRootSel;
class uiSurvInfoProvider;


/*!\brief The main survey selection dialog */

mExpClass(uiIo) uiSurveyManager : public uiDialog
{ mODTextTranslationClass(uiSurveyManager);

public:
			uiSurveyManager(uiParent*);
			~uiSurveyManager();

    /*!\brief Tool item on window. First is always 'X,Y <-> I/C' */
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

    SurveyInfo*		curSurvInfo()		{ return survinfo_; }
    const SurveyInfo*	curSurvInfo() const	{ return survinfo_; }

    const char*		selectedSurveyName() const;
    bool		freshSurveySelected() const
						{ return freshsurveyselected_; }
    bool		haveSurveys() const;

protected:

    SurveyInfo*		survinfo_;
    const BufferString	orgdataroot_;
    BufferString	dataroot_;
    BufferString	initialsurveyname_;
    uiSurveyMap*	survmap_;
    IOPar*		impiop_;
    uiSurvInfoProvider*	impsip_;

    uiDataRootSel*	datarootfld_;
    uiListBox*		survdirfld_;
    uiButton*		editbut_;
    uiButton*		rmbut_;
    ObjectSet<uiButton>	utilbuts_;
    uiTextEdit*		infofld_;
    uiTextEdit*		notesfld_;
    bool		parschanged_; //!< of initial survey only
    bool		cursurvremoved_;
    bool		freshsurveyselected_;

    bool		acceptOK();
    bool		rejectOK();
    void		newButPushed(CallBacker*);
    void		rmButPushed(CallBacker*);
    void		editButPushed(CallBacker*);
    void		copyButPushed(CallBacker*);
    void		extractButPushed(CallBacker*);
    void		compressButPushed(CallBacker*);
    void		odSettsButPushed(CallBacker*);
    void		utilButPushed(CallBacker*);
    void		updateInfoCB( CallBacker* )	{ putToScreen(); }
    void		dataRootChgCB(CallBacker*);
    void		survDirChgCB(CallBacker*);

    void		readSurvInfoFromFile();
    void		setCurrentSurvInfo(SurveyInfo*,bool updscreen=true);
    void		updateDataRootLabel();
    void		updateSurvList();
    void		putToScreen();
    bool		writeSettingsSurveyFile();
    bool		writeSurvInfoFile(bool onlyifcommentchanged);
    bool		rootDirWritable() const;
    bool		doSurvInfoDialog();
    void		updateDataRootInSettings();
    void		rollbackNewSurvey(const uiString&);

private:
    void		fillLeftGroup(uiGroup*);
    void		fillRightGroup(uiGroup*);
};
