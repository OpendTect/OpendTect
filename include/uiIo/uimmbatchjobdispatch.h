#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class Executor;
class HostDataList;
class JobRunner;
class Timer;

class uiButton;
class uiComboBox;
class uiGenInput;
class uiLabel;
class uiListBox;
class uiProgressBar;
class uiSlider;
class uiTextEdit;
class uiTextFileDlg;


mExpClass(uiIo) uiMMBatchJobDispatcher : public uiDialog
{ mODTextTranslationClass(uiMMBatchJobDispatcher);
public:
			~uiMMBatchJobDispatcher();

    static bool		initMMProgram(int argc,char** argv,IOPar& jobpars);

protected:

                        uiMMBatchJobDispatcher(uiParent*,const IOPar&,
                                         const HelpKey& helpkey=mNoHelpKey );

    uiGroup*		specparsgroup_;			// for subclass
    virtual bool	initWork(bool retry)		= 0;
    virtual bool	prepareCurrentJob()		{ return true; }
    virtual Executor*	getPostProcessor() const	{ return 0; }
    virtual bool	haveTmpProcFiles() const	{ return false; }
    virtual bool	removeTmpProcFiles()		{ return true; }
    virtual bool	needConfirmEarlyStop() const	{ return jobrunner_; }
    virtual bool	recoverFailedWrapUp() const	{ return false; }

    IOPar&		jobpars_;
    const HostDataList& hdl_;
    Timer*		timer_ = nullptr;
    int			nrcyclesdone_ = 0;
    uiString		basecaption_;
    JobRunner*		jobrunner_ = nullptr;
    mutable uiString	errmsg_;

    inline bool		isMultiHost() const		{ return avmachfld_; }
    bool		isPaused() const;
    const char*		curUsedMachName();
    int			runnerHostIdx(const char*) const;

private:

    uiListBox*		avmachfld_ = nullptr;
    uiListBox*		usedmachfld_ = nullptr;
    uiTextEdit*		progrfld_ = nullptr;
    uiTextFileDlg*	logvwer_ = nullptr;
    uiSlider*		nicefld_ = nullptr;
    uiProgressBar*	progbar_ = nullptr;
    uiComboBox*		jrppolselfld_;
    uiGenInput*		jrpstartfld_ = nullptr;
    uiGenInput*		jrpstopfld_ = nullptr;
    uiLabel*		jrpworklbl_;
    uiButton*		addbut_;

    bool		retFullFailGoOnMsg();
    bool		rejectOK(CallBacker*) override;
    bool		acceptOK(CallBacker*) override;

    void		initWin(CallBacker*);
    void		doCycle(CallBacker*);
    void		addPush(CallBacker*);
    void		stopPush(CallBacker*);
    void		stopAllPush(CallBacker*);
    void		vwLogPush(CallBacker*);
    void		jrpSel(CallBacker*);
    void		jobPrep(CallBacker*);
    void		jobStart(CallBacker*);
    void		jobFail(CallBacker*);
    void		infoMsgAvail(CallBacker*);

    void		startWork(CallBacker*);
    bool		ready4WrapUp(bool&) const;
    void		handleJobPausing();
    void		updateAliveDisp();
    void		updateCurMachs();
    void		clearAliveDisp();
    bool		wrapUp();
    void		removeTempResults();

};
