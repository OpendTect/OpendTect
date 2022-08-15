#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          October 2009
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "callback.h"
#include "bufstringset.h"
#include "uistring.h"

class uiMainWin;
class Timer;


namespace CmdDrive
{

class CmdDriver;
class CmdRecorder;
class uiCmdDriverDlg;


mExpClass(uiCmdDriver) uiCmdDriverMgr : public CallBacker
{ mODTextTranslationClass(uiCmdDriverMgr);
public:
				uiCmdDriverMgr(bool fullodmode=false);
				~uiCmdDriverMgr();

    void			enableCmdLineParsing(bool yn=true);

    void			addCmdLineScript(const char* fnm);
    void			setLogFileName(const char* fnm);

    void			setDefaultScriptsDir(const char* dirnm);
    void			setDefaultLogDir(const char* dirnm);

    void			showDlgCB(CallBacker*);

    static uiString		usrDispNm() { return tr("Command Driver"); }
    static uiCmdDriverMgr&	getMgr(bool fullodmode=false);

protected:
    void			commandLineParsing();
    void			initCmdLog(const char* cmdlognm);
    void			handleSettingsAutoExec();
    void			delayedStartCB(CallBacker*);
    void			executeFinishedCB(CallBacker*);
    void			autoStart();
    void			timerCB(CallBacker*);
    void			beforeSurveyChg(CallBacker*);
    void			afterSurveyChg(CallBacker*);
    void			stopRecordingCB(CallBacker*);
    void			runScriptCB(CallBacker*);

    void			keyPressedCB(CallBacker*);
    uiCmdDriverDlg*		getCmdDlg();

    uiMainWin&			applwin_;
    CmdDriver*			drv_;
    CmdRecorder*		rec_;
    CmdRecorder*		historec_ = nullptr;
    Timer*			tim_;
    uiCmdDriverDlg*		cmddlg_ = nullptr;

    bool			cmdlineparsing_;
    BufferString		defaultscriptsdir_;
    BufferString		defaultlogdir_;

    BufferStringSet		cmdlinescripts_;
    bool			settingsautoexec_;
    bool			surveyautoexec_;
    int				scriptidx_;
    BufferString		cmdlogname_;
};


} // namespace CmdDrive


