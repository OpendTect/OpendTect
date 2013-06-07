#ifndef cmddrivermgr_h
#define cmddrivermgr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          October 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "callback.h"
#include "bufstringset.h"

class uiPopupMenu;
class uiMainWin;
class Timer;
class uiToolBar;
class uiMenuItem;
class uiMenuItem;

namespace CmdDrive
{

class CmdDriver;
class CmdRecorder;
class uiCmdDriverDlg;


mClass uiCmdDriverMgr : public CallBacker
{
public:
				uiCmdDriverMgr(bool fullodmode=false);
				~uiCmdDriverMgr();

    void			enableCmdLineParsing(bool yn=true);

    void			addCmdLineScript(const char* fnm);
    void			setLogFileName(const char* fnm);

    void			setDefaultScriptsDir(const char* dirnm);
    void			setDefaultLogDir(const char* dirnm);

    void			showDlgCB(CallBacker*);

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

    void                	closeDlg(CallBacker*);
    void			keyPressedCB(CallBacker*);
    uiCmdDriverDlg*		getCmdDlg();

    uiMainWin&           	applwin_;
    CmdDriver*			drv_;
    CmdRecorder*		rec_;
    CmdRecorder*		historec_;
    Timer*			tim_;
    uiMenuItem*			cmdmnuitm_;
    uiCmdDriverDlg*		cmddlg_;

    bool			cmdlineparsing_;
    BufferString		defaultscriptsdir_;
    BufferString		defaultlogdir_;

    BufferStringSet		cmdlinescripts_;
    bool			settingsautoexec_;
    bool			surveyautoexec_;
    int				scriptidx_;
    BufferString		cmdlogname_;
};


}; // namespace CmdDrive


#endif

