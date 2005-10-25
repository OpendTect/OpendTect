#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:           $Id: uiseismmproc.h,v 1.34 2005-10-25 11:46:59 cvsarend Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class IOPar;
class IOObj;
class Timer;
class uiLabel;
class Executor;
class uiSlider;
class JobRunner;
class IOParList;
class uiTextEdit;
class uiGenInput;
class uiComboBox;
class HostDataList;
class uiProgressBar;
class uiFileBrowser;
class uiIOFileSelect;
class uiSeisIOObjInfo;
class SeisJobExecProv;
class uiLabeledListBox;


class uiSeisMMProc : public uiDialog
{
public:
                        uiSeisMMProc(uiParent*,const char* prognm,
				     const IOParList&);
			~uiSeisMMProc();

protected:

    HostDataList&	hdl;
    SeisJobExecProv*	jobprov;
    JobRunner*		jobrunner;
    Executor*		task;
    Timer*		timer;
    IOParList&		iopl;
    BufferString	progname;
    int			nrcyclesdone;
    uiSeisIOObjInfo*	outioobjinfo;
    bool		isrestart;
    bool		is2d;
    bool 		paused;
    bool 		lsfileemitted;

    uiLabeledListBox*	avmachfld;
    uiLabeledListBox*	usedmachfld;
    uiComboBox*		jrppolselfld;
    uiIOFileSelect*	tmpstordirfld;
    uiGenInput*		inlperjobfld;
    uiTextEdit*		progrfld;
    uiFileBrowser*	logvwer;
    uiGenInput*		jrpstartfld;
    uiGenInput*		jrpstopfld;
    uiLabel*		jrpworklbl;
    uiSlider*		nicefld;
    uiProgressBar*	progbar;
    BufferString	caption;

    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

    void		initWin(CallBacker*);
    void		doCycle(CallBacker*);
    void		addPush(CallBacker*);
    void		stopPush(CallBacker*);
    void		vwLogPush(CallBacker*);
    void		jobPrepare(CallBacker*);
    void		jobStarted(CallBacker*);
    void		jobFailed(CallBacker*);
    void		infoMsgAvail(CallBacker*);
    void		jrpSel(CallBacker*);

    void		startWork(CallBacker*);
    void		setNiceNess();
    void		updateAliveDisp();
    void		updateCurMachs();
    bool		wrapUp(bool);
    bool		readyForPostProcess();
    int			runnerHostIdx(const char*) const;

    const char* 	curUsedMachName();
    void		pauseJobs();
    int			defltNrInlPerJob(const IOPar&);
    void		mkJobRunner(int);
};

#endif
