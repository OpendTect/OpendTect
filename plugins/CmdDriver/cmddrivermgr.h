#ifndef cmddrivermgr_h
#define cmddrivermgr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          October 2009
 RCS:           $Id: cmddrivermgr.h,v 1.6 2011/05/06 06:43:27 cvsjaap Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "bufstringset.h"

class uiPopupMenu;
class uiODMain;
class Timer;
class uiToolBar;
class uiMenuItem;
class uiMenuItem;

namespace CmdDrive
{

class CmdDriver;
class CmdRecorder;
class uiCmdDriverDlg;


mClass uiCmdDriverMgr:public CallBacker
{
public:
				uiCmdDriverMgr(uiODMain&);
				~uiCmdDriverMgr();
protected:
    void			commandLineParsing();
    void			initCmdLog(const char* cmdlognm);
    void			handleSettingsAutoExec();
    void			delayedStartCB(CallBacker*);
    void			executeFinishedCB(CallBacker*);
    void			dlgCB(CallBacker*);
    void			autoStartCB(CallBacker*);
    void			beforeSurveyChg(CallBacker*);
    void			afterSurveyChg(CallBacker*);

    void                	closeDlg(CallBacker*);
    void			keyPressedCB(CallBacker*);
    uiCmdDriverDlg*		getCmdDlg();

    uiODMain&           	appl_;
    CmdDriver*			drv_;
    CmdRecorder*		rec_;
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
