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

#include "uiiomod.h"
#include "uidialog.h"
#include "bufstring.h"

class BufferStringSet;
class SurveyInfo;
class uiButton;
class uiLabel;
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

    SurveyInfo*		curSurvInfo()		{ return cursurvinfo_; }
    const SurveyInfo*	curSurvInfo() const	{ return cursurvinfo_; }

    const char*		selectedSurveyName() const;

protected:

    SurveyInfo*		cursurvinfo_;
    const BufferString	orgdataroot_;
    BufferString	dataroot_;
    BufferString	initialsurveyname_;
    uiSurveyMap*	survmap_;
    IOPar*		impiop_;
    uiSurvInfoProvider*	impsip_;

    uiLabel*		datarootlbl_;
    uiListBox*		dirfld_;
    uiButton*		editbut_;
    uiButton*		rmbut_;
    ObjectSet<uiButton>	utilbuts_;
    uiTextEdit*		infofld_;
    uiTextEdit*		notesfld_;
    bool		parschanged_; //!< of initial survey only
    bool		cursurvremoved_;

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
    void		selChange(CallBacker*);
    void		updateInfo( CallBacker* )	{ putToScreen(); }

    void		readSurvInfoFromFile();
    void		setCurrentSurvInfo(SurveyInfo*,bool updscreen=true);
    void		updateDataRootLabel();
    void		updateSurvList();
    void		putToScreen();
    bool		writeSettingsSurveyFile();
    bool		writeSurvInfoFileIfCommentChanged();
    bool		rootDirWritable() const;
    bool		doSurvInfoDialog(bool isnew);
    void		updateDataRootInSettings();
    void		rollbackNewSurvey(const char*);

private:
    void		fillLeftGroup(uiGroup*);
    void		fillRightGroup(uiGroup*);
};

#endif
