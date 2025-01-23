#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    static bool		ensureGoodSurveySetup(uiRetVal&,uiParent* =nullptr);
    static bool		ensureValidDataRoot(uiRetVal&,uiParent* =nullptr);
    mDeprecated("Provide uiRetVal")
    static bool		ensureValidDataRoot();
    enum SurvSelState	{ InvalidSurvey, SameSurvey, NewExisting,
			  NewFresh, SurveyRemoved };
    static SurvSelState ensureValidSurveyDir(uiRetVal&,uiParent* =nullptr);
    static SurvSelState& lastSurveyState();

    /*!\brief 'Menu' item on window. First is always 'X,Y <-> I/C' */
    struct Util
    {
			Util( const char* pixmap, const uiString& tooltip,
				const CallBack& cb )
			    : cb_(cb)
			    , pixmap_(pixmap)
			    , tooltip_(tooltip)		{}
			~Util()				{}

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
    StringPairSet	infoset_;

    bool		parschanged_; //!< of initial survey only
    bool		cursurvremoved_;
    bool		freshsurveyselected_;

    bool		acceptOK(CallBacker*) override;
    bool		rejectOK(CallBacker*) override;
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

    static bool		Convert_OD4_Data_To_OD5();
    static bool		Convert_OD4_Body_To_OD5();
};


//--- uiStartNewSurveySetup


mExpClass(uiIo) uiStartNewSurveySetup : public uiDialog
{ mODTextTranslationClass(uiStartNewSurveySetup);

public:
			uiStartNewSurveySetup(uiParent*,const char*,
					      SurveyInfo&);
			~uiStartNewSurveySetup();

    void		setSurveyNameFld(BufferString,bool);
    bool		isOK();
    bool		acceptOK(CallBacker*) override;

    ObjectSet<uiSurvInfoProvider> sips_;
    int			sipidx_;

private:

    const BufferString	dataroot_;
    SurveyInfo&		survinfo_;
    uiGenInput*		survnmfld_;
    uiGenInput*		zistimefld_;
    uiGenInput*		zinfeetfld_;
    uiListBox*		sipfld_;

    BufferString	sipName() const;
    BufferString	survName() const;
    bool		isTime() const;
    bool		isInFeet() const;
    void		fillSipsFld();
    void		zdomainChg(CallBacker*);
    void		sipCB(CallBacker*);

};

#define mIfIOMNotOK( act ) \
    if ( !uiSurvey::ensureGoodSurveySetup(uirv) ) \
	{ if ( !uirv.isOK() ) uiMSG().error( uirv ); act; }
