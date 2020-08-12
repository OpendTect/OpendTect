#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          April 2002
 RCS:           $Id$
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
                        uiMMBatchJobDispatcher(uiParent*,const IOPar&,
                                         const HelpKey& helpkey=mNoHelpKey );
			~uiMMBatchJobDispatcher();

    static bool		initMMProgram(int argc,char** argv,IOPar& jobpars);

protected:

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
    Timer*		timer_;
    int			nrcyclesdone_;
    uiString		basecaption_;
    JobRunner*		jobrunner_;
    mutable uiString	errmsg_;

    inline bool		isMultiHost() const		{ return avmachfld_; }
    bool		isPaused() const;
    const char*		curUsedMachName();
    int			runnerHostIdx(const char*) const;

private:

    uiListBox*		avmachfld_;
    uiListBox*		usedmachfld_;
    uiTextEdit*		progrfld_;
    uiTextFileDlg*	logvwer_;
    uiSlider*		nicefld_;
    uiProgressBar*	progbar_;
    uiComboBox*		jrppolselfld_;
    uiGenInput*		jrpstartfld_;
    uiGenInput*		jrpstopfld_;
    uiLabel*		jrpworklbl_;
    uiButton*		addbut_;

    bool		retFullFailGoOnMsg();
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

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

