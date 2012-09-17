#ifndef uiseismmproc_h
#define uiseismmproc_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          April 2002
 RCS:           $Id: uiseismmproc.h,v 1.39 2009/07/22 16:01:23 cvsbert Exp $
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
class uiTextEdit;
class uiGenInput;
class uiComboBox;
class HostDataList;
class uiProgressBar;
class uiTextFileDlg;
class uiIOFileSelect;
class uiSeisIOObjInfo;
class SeisJobExecProv;
class uiLabeledListBox;


mClass uiSeisMMProc : public uiDialog
{
public:
                        uiSeisMMProc(uiParent*,const IOPar&,
				     const char* prognm,const char* parfnm);
			~uiSeisMMProc();

protected:

    HostDataList&	hdl;
    SeisJobExecProv*	jobprov;
    JobRunner*		jobrunner;
    Executor*		task;
    Timer*		timer;
    const IOPar&	iop;
    BufferString	progname;
    BufferString	parfnm;
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
    uiTextFileDlg*	logvwer;
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
