#ifndef cmddrivermgr_h
#define cmddrivermgr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          October 2009
 RCS:           $Id: uicmddrivermgr.h,v 1.1 2012-09-17 12:38:34 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
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


mClass(CmdDriver) uiCmdDriverMgr:public CallBacker
{
public:
				uiCmdDriverMgr(uiMainWin& applwin);
				~uiCmdDriverMgr();

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

    BufferStringSet		cmdlinescripts_;
    bool			settingsautoexec_;
    bool			surveyautoexec_;
    int				scriptidx_;
    BufferString		cmdlogname_;
};


}; // namespace CmdDrive


#endif

