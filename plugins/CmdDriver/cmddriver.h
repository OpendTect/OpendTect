#ifndef cmddriver_h
#define cmddriver_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Sep 2005
 RCS:           $Id: cmddriver.h,v 1.69 2011/09/05 14:40:51 cvsjaap Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include <iostream>
#include "separstr.h"
#include "strmdata.h"
#include "thread.h"
#include "uimainwin.h"

#include "cmddriverbasics.h"
#include "searchkey.h"

class FilePath;
class StreamData;
class uiObject;
class uiPopupMenu;
class uiTaskRunner;

namespace CmdDrive
{
    class Activator;
    class IdentifierManager;
    class ExprInterpreter;
    class WildcardManager;
    class MenuTracer;

#define mLogStrm \
    if ( drv_.logStrmData().usable() ) *drv_.logStrmData().ostrm

#define mTimeStrm \
    mLogStrm << "[" << Time::getTimeString() << "]\t"

#define mParseMsgStrm(tag) \
    if ( !drv_.streamBlocked(true,tag) ) \
	mLogStrm << "\t" << (tag) << " [parse]: "

#define mParseStrm(warn)	mParseMsgStrm( warn ? "WARN" : "ERROR" )
#define mParseErrStrm		mParseStrm( false )
#define mParseWarnStrm		mParseStrm( true )

#define mWinMsgStrm(tag) \
    if ( !drv_.streamBlocked(false,tag) ) \
	mLogStrm << "\t" << (tag) << " [" << drv_.curWinTitle() << "]:\n\t\t"

#define mWinStrm(warn)  mWinMsgStrm( warn ? "WARN " : "ERROR" )
#define mWinErrStrm     mWinStrm( false )
#define mWinWarnStrm    mWinStrm( true )


mClass ModalStatus
{
public:
    bool			operator==(const ModalStatus&) const;
    bool			operator!=(const ModalStatus&) const;
    void			get();

    uiMainWin::ActModalTyp	activetype_;
    uiMainWin*			activewin_;
    uiTaskRunner*		uitaskrunner_;
    int				nrmodalwins_;
    BufferStringSet		signatures_;			
};


mClass Action
{
public:
    				Action(const char* line)
				    : line_(line), gotoidx_(-1)
				    , insertidx_(-1)			{}

    BufferString		line_;
    int				gotoidx_;
    int				insertidx_;
};


mClass MenuInfo
{
public:
    int				nrchildren_;
    int				siblingnr_;
    int				ison_;
    BufferString		text_;
};


enum WinStateType { NoState=0, Existent, Inexistent, Accessible, Inaccessible };


mClass CmdDriver : public CallBacker
{
public:
    friend class 	Command;
    friend class 	ExprInterpreter;
    friend class 	Function;
    friend class 	MenuTracer;

    			CmdDriver();
    			~CmdDriver();

    bool		getActionsFromFile(const char*);
    bool		insertActionsFromFile(const char*);

    bool		execute();
    void		abort()				{ abort_ = true; }
    void		pause(bool yn=true);
    const char*		errMsg() const			{ return errmsg_.str();}

    const char*		outputDir() const		{ return outdir_; }
    void		setOutputDir(const char* od)	{ outdir_ = od; }

    static const char*	defaultLogFilename();
    const char*		logFileName() const		{ return logfnm_; }
    void		setLogFileName(const char* fnm)	{ logfnm_ = fnm; }
    void		clearLog()			{ logsd_.close(); }

    enum		LogModeTag { LogBasic, LogNormal, LogAll };		
    void		setLogMode(LogModeTag tag)	{ logmode_ = tag; } 

    enum		OnErrorTag { Stop, Recover };
    void		setOnError(OnErrorTag tag)	{ onerror_ = tag; }

    void		setCaseSensitive(bool yn)   { casesensitive_ = yn; }
    void		skipGreyOuts(bool yn=true)  { skipgreyouts_ = yn; }

    bool		isCaseSensitive() const     { return casesensitive_; }
    OnErrorTag		onError() const		    { return onerror_; }
    bool		greyOutsSkipped() const	    { return skipgreyouts_; }

    void		setSleep(float time,
	    			 bool regular=true);
    void		setWait(float time,
	    			bool regular=true);

    static bool		nowExecuting();

    enum		RecoveryTag { NoClue, CloseCurWin, NextCmd,
				      NextAssertion };

    enum		InterceptMode { Click, ItemInfo, NodeInfo };

    Notifier<CmdDriver>	interactRequest;
    Notifier<CmdDriver>	executeFinished;

protected:

    const uiObject*	localSearchEnv() const      { return localsearchenv_; }

    bool		doLocalAction(uiObject* localenv,const char* actstr);
    bool		tryAction(const char* identname,const char* actstr);

    void		prepareForResume();
    void		prepareForAbort();

    bool		abort_;
    bool		pause_;
    bool		resume_;

    bool		prepareActivate(Activator*);
    void		finishActivate(bool busywait=true);
    void		waitForClearance();

    void		prepareIntercept(const FileMultiString& mnupath,
	    				 int onoff,InterceptMode=Click);
    bool		didInterceptSucceed(const char* objnm);
    const MenuInfo&	interceptedMenuInfo() const;

    void		reInit();

    CmdDriver&		drv_;
    BufferString	outdir_;
    BufferString	logfnm_;
    BufferString	cmdfnm_;
    BufferString	errmsg_;
    Threads::Thread*	execthr_;
    StreamData&		logsd_;
    FilePath&		outfp_;

    WildcardManager*	wcm_;
    WildcardManager&	wildcardMan()			{ return *wcm_; }

    IdentifierManager*	idm_;
    IdentifierManager&	identifierMan()			{ return *idm_; }

    const WildcardManager&	wildcardMan() const;
    const IdentifierManager&	identifierMan() const;

    ObjectSet<const char> tryoutstack_;
    int			tryoutval_;

    ExprInterpreter*	eip_;
    ExprInterpreter&	exprInterpreter()		{ return *eip_; }

    void		logErrMsg();
    void		updateLogStrm();
    LogModeTag		logmode_;

    bool		addActions(ObjectSet<Action>&,const char*);
    ObjectSet<Action>	actions_;
    int			actionidx_;

    int			curActionIdx() const		{ return actionidx_; }
    bool		insertProcedure(int defidx);

    void		moveActionIdx(int nrlines);
    int			lastActionIdxMove() const	{ return lastmove_; }
    int			lastmove_;

    void		end();
    void		jump(int extralines=0);
    bool		curactjumped_;

    void		mkThread(CallBacker*);
    bool		doAction(const char*);

    bool		casesensitive_;	
    uiObject*		localsearchenv_;

    bool		skipgreyouts_;
    bool		uiobjchange_;
    bool		goingToChangeUiObj() const	{ return uiobjchange_; }

    float		pendingsleep_;
    float		regularsleep_;

    float		pendingwait_;
    float		regularwait_;

    BufferString	winassertion_;
    bool		winassertcs_;
    bool		winassertsafe_;
    bool		verifyWinAssert(const char* newwinstr=0);

    BufferString	winstatewin_;
    WinStateType	winstatetype_;
    bool		verifyWinState(const char* newwinstr=0,
				       WinStateType newwinstate=NoState);

    OnErrorTag		onerror_;
    bool		recover();
    RecoveryTag		recoverystep_;
    void		setRecoveryStep(RecoveryTag rt) { recoverystep_ = rt; }

    static const char*	locateCmdMark(const char* actstr);

    bool		switchCurWin(const uiMainWin*);
    WindowStack		winstack_;
    const uiMainWin*	curWin() const		{ return winstack_.topWin(); }

    bool		openqdialog_; 
    bool		openQDlg() const		{ return openqdialog_; }
    
    Threads::Mutex	cmddrvmutex_;
    bool		activityStopped(bool checkprocessing=true,
					bool checktimers=true);

    bool		waitForTimers();
    bool		waitForProcessing();
    int			wildmodalclosedstamp_;

    void		storeModalStatus();
    BufferStringSet	wildmodalsignatures_;

    ModalStatus		curmodalstat_;
    ModalStatus		prevmodalstat_;
    int			prevnrproc_;

    void		activateDone(CallBacker* activator);
    void		timerStartsCB(CallBacker* timer);
    void		timerStoppedCB(CallBacker* timer);
    void		timerShootsCB(CallBacker* timer);
    void		timerShotCB(CallBacker* timer);

    void 		forceQtToCatchUp() const;
			// Delay to assure that Qt has finished all previous 
			// display, hide and outgrey tasks initiated by the
			// GUI thread.

    enum		InterceptStatus { NoInterception, InterceptError,
					  InterceptReady };
    InterceptStatus	interceptstatus_;  
    FileMultiString	interceptmenupath_;
    InterceptMode	interceptmode_;
    int			interceptonoff_;  
    bool		interceptmenu_;
    MenuInfo		interceptmenuinfo_;

    void		dynamicMenuInterceptor(CallBacker*);
    bool		dispatchDynamicMenu(uiPopupMenu*);

    void		interactCB(CallBacker*);
    void		executeFinishedCB(CallBacker*);
    void		killTaskRunnerCB(CallBacker*);

    const InteractSpec*	interactspec_;
    void		interact(const InteractSpec*); 

    ObjectSet<const CallBacker>	activatorlist_;
    ObjectSet<const CallBacker>	timerlist_;
    ObjectSet<const CallBacker>	timeoutlist_;


public:
    			// interface for output stream macros
    const StreamData&	logStrmData() const		    { return logsd_; }
    const char*		curWinTitle(int aliasnr=0) const;

    bool		streamBlocked(bool parse,const char* tag);
    			// Does tryout management as side-effect!

};


}; // namespace CmdDrive


#endif
