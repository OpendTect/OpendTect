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
namespace File { class Monitor; }


/*!\brief The main survey selection dialog.

  When run in 'standalone' mode survey and data root changes can be done at any
  time. Inside od_main or other programs that can have background taks, this
  is not a good idea. The updating while on screen then relies on monitoring
  survey directories and files.

 */

mExpClass(uiIo) uiSurveyManager : public uiDialog
{ mODTextTranslationClass(uiSurveyManager);

public:
			uiSurveyManager(uiParent*,bool standalone);
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

    const SurveyInfo*	curSurvInfo() const	{ return curSI(); }

    const char*		selectedSurveyName() const;
    bool		haveSurveys() const;

protected:

    bool		isStandAlone() const	{ return !survinfo_; }
    const SurveyInfo*	curSI() const;

    SurveyInfo*		survinfo_;
    BufferString	dataroot_;
    uiSurveyMap*	survmap_;
    File::Monitor*	filemonitor_;
    uiRetVal		survreadstatus_;

    uiDataRootSel*	datarootfld_;
    uiListBox*		survdirfld_;
    uiButton*		editbut_;
    uiButton*		rmbut_;
    ObjectSet<uiButton>	utilbuts_;
    uiTextEdit*		infofld_;
    uiTextEdit*		notesfld_;
    uiString		rootdirnotwritablestr_;

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
    void		dataRootChgCB(CallBacker*);
    void		survDirChgCB(CallBacker*);
    void		survParFileChg(CallBacker*);
    void		dataRootDirChgCB(CallBacker*)	{ updateSurvList(); }

    void		reReadSurvInfoFromFile(const char*);
    void		setCurrentSurvey(const char*);
    bool		writeSettingsSurveyFile(const char*);
    void		updateSurvList();
    void		startFileMonitoring();
    void		stopFileMonitoring();
    void		putToScreen();
    void		launchEditor(bool);
    void		writeCommentsIfChanged();
    bool		rootDirWritable() const;

private:

    void		fillLeftGroup(uiGroup*);
    void		fillRightGroup(uiGroup*);

};
