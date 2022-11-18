/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "cmddriver.h"

#include "command.h"
#include "identifierman.h"
#include "interpretexpr.h"
#include "cmdfunction.h"

#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "od_istream.h"
#include "thread.h"
#include "timefun.h"
#include "timer.h"
#include "genc.h"

#include "uicursor.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uistrings.h"
#include "uitaskrunner.h"


namespace CmdDrive
{


class ModalStatusActivator: public Activator
{
public:
		ModalStatusActivator( ModalStatus& ms )
		    : modalstat_( ms )			{}
    void	actCB(CallBacker*) override;

protected:
    ModalStatus& modalstat_;
};


void ModalStatusActivator::actCB( CallBacker* cb )
{
    BufferStringSet startsignatures;
    uiMainWin::getModalSignatures( modalstat_.signatures_ );

    do
    {
	startsignatures = modalstat_.signatures_;
	modalstat_.activewin_ = uiMainWin::activeModalWindow();
	modalstat_.activetype_ = uiMainWin::activeModalType();

	uiMainWin::getModalSignatures( modalstat_.signatures_ );
	modalstat_.nrmodalwins_ = modalstat_.signatures_.size();

	mDynamicCast( uiTaskRunner*,
		      modalstat_.uitaskrunner_, modalstat_.activewin_ );
    }
    while ( !modalstat_.signatures_.isSubsetOf(startsignatures) ||
	    !startsignatures.isSubsetOf(modalstat_.signatures_) );
}


bool ModalStatus::operator==( const ModalStatus& ms ) const
{
    return activetype_==ms.activetype_ &&
	   activewin_==ms.activewin_ &&
	   uitaskrunner_==ms.uitaskrunner_ &&
	   nrmodalwins_==ms.nrmodalwins_ &&
	   signatures_.isSubsetOf(ms.signatures_) &&
	   ms.signatures_.isSubsetOf(signatures_);
}


bool ModalStatus::operator!=( const ModalStatus& ms ) const
{ return  !operator==( ms ); }


#define mBusyWaitUntil( condition ) \
{ \
    float minsleep = 0.01; \
    const float maxsleep = 1.28; \
    while ( applwin_ && !(condition) ) \
    { \
	sleepSeconds( minsleep ); \
	if ( minsleep < maxsleep ) \
	    minsleep *= 2; \
    } \
}


static int getNrActiveCmdDrivers( int nrextra )
{
    mDefineStaticLocalObject( int, nrcmddrvrs, = 0 );
    nrcmddrvrs += nrextra;
    return nrcmddrvrs;
}


const char* CmdDriver::defaultLogFilename()
{ return "odcmdlog.txt"; }


CmdDriver::CmdDriver( uiMainWin& aw )
	: drv_(*this)
	, applwin_( &aw )
        , wcm_( new WildcardManager() )
	, idm_( new IdentifierManager() )
	, eip_( new ExprInterpreter(*this) )
	, logstream_(*new od_ostream)
	, outfp_(*new FilePath)
	, outdir_(GetPersonalDir())
	, logfnm_(defaultLogFilename())
	, execthr_(0)
	, executeFinished(this)
	, interactRequest(this)
{
    Command::initStandardCommands();
    Function::initStandardFunctions();
    reInit();
}


void CmdDriver::clearLog()
{
    logstream_.close();
}


const WildcardManager& CmdDriver::wildcardMan() const
{ return const_cast<const WildcardManager&>( *wcm_ ); }

const IdentifierManager& CmdDriver::identifierMan() const
{ return const_cast<const IdentifierManager&>( *idm_ ); }


void CmdDriver::reInit()
{
    logmode_ = LogNormal;
    casesensitive_ = false;
    skipgreyouts_ = true;
    uiobjchange_ = false;
    onerror_ = Recover;

    abort_ = false;
    pause_ = false;
    resume_ = false;
    failed_ = false;

    regularsleep_ = 0;
    pendingsleep_ = 0;
    regularwait_ = 3.0;
    pendingwait_ = regularwait_;

    openqdialog_ = false;
    localsearchenv_ = 0;
    tryoutstack_.erase();
    interceptmenu_ = false;
    recoverystep_ = NoClue;

    winassertion_.setEmpty();
    winassertcs_ = false;
    winassertsafe_ = true;

    winstatewin_.setEmpty();
    winstatetype_ = NoState;

    winstack_.synchronize();
    deepErase( activatorlist_ );
    timerlist_.erase();
    timeoutlist_.erase();
    errmsg_.setEmpty();

    interactspec_ = 0;
    lastmove_ = 1;
}


CmdDriver::~CmdDriver()
{
    deepErase( activatorlist_ );
    deepErase( actions_ );

    logstream_.close();
    delete &logstream_;
    delete &outfp_;

    if ( execthr_ )
    {
	execthr_->waitForFinish();
	delete execthr_;
    }

    delete wcm_;
    delete idm_;
    delete eip_;
}


bool CmdDriver::nowExecuting()
{
    return getNrActiveCmdDrivers(0) > 0;
}


bool CmdDriver::getActionsFromFile( const char* fnm )
{
    if ( nowExecuting() )
	return false;

    deepErase( actions_ );
    idm_->reInit();
    cmdfnm_ = fnm;

    if ( !addActions(actions_,fnm) )
    {
	logErrMsg();
	return false;
    }

    return true;
}


bool CmdDriver::insertActionsFromFile( const char* fnm )
{
    if ( !nowExecuting() )
	return false;

    ObjectSet<Action> newactions;
    if ( !addActions(newactions,fnm) )
    {
	logErrMsg();
	return false;
    }

    if ( newactions.isEmpty() )
	return true;

    newactions[newactions.size()-1]->insertidx_ = actionidx_ + 1;

    Threads::Locker lckr( cmddrvlock_ );
    for ( int idx=newactions.size()-1; idx>=0; idx-- )
    {
	if ( newactions[idx]->gotoidx_ >= 0 )
	    newactions[idx]->gotoidx_ += actionidx_ + 1;

	actions_.insertAt( newactions[idx], actionidx_+1 );
    }
    lckr.unlockNow();

    winassertsafe_ = false;
    return true;
}


#define mPreProcSubstitution( nextword, linenr, action ) \
{ \
    BufferString temp; \
    int nrsubst = idm_->substitute( action, temp ); \
    action = temp; \
    nextword = getNextNonBlanks( action, temp.getCStr() ); \
    if ( nrsubst < 0 ) \
    { \
	errmsg_ += "has "; errmsg_ += (-nrsubst); errmsg_ += " substitution "; \
	errmsg_ += nrsubst<-1 ? "failures" : "failure"; \
	errmsg_ += " at line "; errmsg_ += linenr; errmsg_ +=": '"; \
	errmsg_ += action; errmsg_ += "'"; \
	return false; \
    } \
}

#define mCheckTail( tail, linenr, action ) \
{ \
    mSkipBlanks( tail ); \
    if ( !tail || *tail ) \
    { \
	errmsg_ += "has unexpected content at end of line "; \
	errmsg_ += linenr; errmsg_ += ": '"; \
	errmsg_ += action; errmsg_ += "'"; \
	return false; \
    } \
}


#define mCheckFlowStart( cmd ) \
\
    if ( mMatchCI(cmdname, #cmd) ) \
    { \
	flowstack.insert( 0, cmd##Tag ); \
	actidxstack.insert( 0, actionlist.size()-1 ); \
    }

#define mCheckFlow( cmd, condition, forward, backward, matchingstr ) \
\
    if ( mMatchCI(cmdname, #cmd) ) \
    { \
	int* actidxptr = 0; \
	for ( int idx=0; idx<flowstack.size(); idx++ ) \
	{ \
	    if ( flowstack[idx] condition ) \
		actidxptr = &actidxstack[idx]; \
	    if ( actidxptr || forward ) \
		break; \
	} \
	if ( !actidxptr ) \
	{ \
	    errmsg_ += "has syntax error at line "; errmsg_ += linenr; \
	    errmsg_ += ": '"; errmsg_ += #cmd; \
	    errmsg_ += "' - Matching '"; errmsg_ += matchingstr; \
	    errmsg_ += "' is missing or ill-formatted"; \
	    return false; \
	} \
	if ( backward ) \
	    actionlist[actionlist.size()-1]->gotoidx_ = *actidxptr; \
	\
	if ( forward ) \
	{ \
	    actionlist[*actidxptr]->gotoidx_ = actionlist.size()-1; \
	    *actidxptr = actionlist.size()-1; \
	    if ( backward ) actidxstack.removeSingle( 0 ); \
	    if ( backward ) flowstack.removeSingle( 0 ); \
	} \
	if ( StringView(#cmd) == "Else" ) \
	    flowstack[0] = ElseTag; \
    }

#define mCheckFlowStack( stack ) \
{ \
    if ( !stack.isEmpty() ) \
    { \
	errmsg_ += "ended in the middle of "; \
	errmsg_ += stack[0]==DefTag ? "a procedure def-inition" : \
		   stack[0]==ForTag ? "a for-loop" : \
		   stack[0]==DoWhileTag ? "a while-loop" : \
		   stack[0]==DoTag ? "an until-loop" : "an if-structure"; \
	return false; \
    } \
}


bool CmdDriver::getIncludedScripts( const char* fnm,
				    BufferStringSet& incs ) const
{
    errmsg_ = "Include finder. File: ";
    errmsg_.add( fnm ).addNewLine();
    od_istream strm( fnm );
    if ( !strm.isOK() )
	{ errmsg_ += "File cannot be opened"; return false; }

    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType("OpendTect commands") )
	{ errmsg_ += "File is invalid"; return false; }

    BufferString line, word;
    incs.erase();
    while ( strm.isOK() )
    {
	strm.getLine( line );
	char* lineptr = line.getCStr();
	mTrimBlanks( lineptr );
	if ( !*lineptr )
	    continue;

	const char* ptr = getNextWord( lineptr, word.getCStr() );
	if ( caseInsensitiveEqual(word,"include") )
	{
	    getNextWord( ptr, word.getCStr() );
	    BufferString filepath;
	    idm_->substitute( word.buf(), filepath );
	    incs.add( filepath );
	}
    }

    return !incs.isEmpty();
}


bool CmdDriver::addActions( ObjectSet<Action>& actionlist, const char* fnm )
{
    errmsg_ = actionlist.isEmpty() ? "Command" : "Included command";
    errmsg_ += " file \""; errmsg_ += fnm; errmsg_ += "\" ";

    od_istream strm( fnm );
    if ( !strm.isOK() )
	{ errmsg_ += "cannot be opened"; return false; }

    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType("OpendTect commands") )
	{ errmsg_ += "is invalid"; return false; }

    int linenr = 4;		// Header has four lines
    int extralines = 0;

    enum FlowStackTag { IfTag, ElseTag, DefTag, DoTag, DoWhileTag, ForTag };
    TypeSet<int> flowstack, actidxstack;

    BufferString line, actseq, action;
    const char* actptr = actseq.buf();
    line.setBufSize( 8096 );

    while ( strm.isOK() )
    {
	if ( !*actptr )
	{
	    linenr += 1 + extralines;
	    extralines = 0;
	    actseq.setEmpty();

	    while ( strm.isOK() )
	    {
		strm.getLine( line );
		char* lineptr = line.getCStr();
		mTrimBlanks( lineptr );
		if ( !*lineptr )
		    break;

		actseq += lineptr;
		if ( !StringProcessor(actseq).removeTokenAppendix('\\') )
		    break;

		extralines++;
	    }
	    actptr = actseq.buf();
	}

	actptr = StringProcessor(actptr).nextAction( action );
	mSkipBlanks( actptr );
	BufferString firstword;
	const char* nextword = getNextNonBlanks( action, firstword.getCStr() );

	if ( firstword.isEmpty() || firstword[0]=='#' )
	    continue;

	if ( mMatchCI(firstword,"Include") )
	{
	    mPreProcSubstitution( nextword, linenr, action );
	    BufferString inclfnm;
	    const char* tail = StringProcessor(nextword).parseDQuoted(inclfnm);
	    StringProcessor(inclfnm).makeDirSepIndep();
	    mCheckTail( tail, linenr, action );

	    BufferString errmsgprefix = errmsg_;
	    if ( !addActions(actionlist,inclfnm) )
		return false;

	    errmsg_ = errmsgprefix;
	    continue;
	}

	actionlist += new Action( action );

	const char* cmdmark = locateCmdMark( action );
	BufferString cmdname;
	getNextNonBlanks( cmdmark, cmdname.getCStr() );

	mCheckFlowStart( If );
	mCheckFlowStart( Do );
	mCheckFlowStart( DoWhile );
	mCheckFlowStart( For );
	mCheckFlowStart( Def );

	mCheckFlow( ElseIf,   ==IfTag, true, false, "If" );
	mCheckFlow( Else,     ==IfTag, true, false, "If" );
	mCheckFlow( Fi,     <=ElseTag, true,  true, "If" );
	mCheckFlow( OdUntil,  ==DoTag, true,  true, "Do" );
	mCheckFlow( Od,  ==DoWhileTag, true,  true, "DoWhile" );
	mCheckFlow( Rof,     ==ForTag, true,  true, "For" );
	mCheckFlow( Fed,     ==DefTag, true,  true, "Def" )
	mCheckFlow( Break,    >=DoTag, false, true, "Do', 'DoWhile' or 'For" );
	mCheckFlow( Continue, >=DoTag, false, true, "Do', 'DoWhile' or 'For" );
	mCheckFlow( Return,  ==DefTag, false, true, "Def" );
    }

    mCheckFlowStack( flowstack );
    errmsg_.setEmpty();
    return true;
}


void CmdDriver::logErrMsg()
{
    if ( errmsg_.isEmpty() )
	return;

    updateLogStrm();
    mLogStrm << od_newline << errmsg_ << od_newline << od_endl;
}


void CmdDriver::updateLogStrm()
{
    if ( outdir_.isEmpty() || !File::isDirectory(outdir_) )
	outfp_.set( GetProcFileName(logfnm_) );
    else
	outfp_.set(outdir_).add( logfnm_ );

    BufferString prevlogfnm = logstream_.fileName();

    if ( !logstream_.isOK() || prevlogfnm!=outfp_.fullPath() )
    {
        logstream_.close();
        logstream_.open( outfp_.fullPath() );
    }
}


bool CmdDriver::execute()
{
    updateLogStrm();

    mLogStrm << od_newline << "Command file: " << cmdfnm_ << od_newline
	     << "Execution started at " << Time::getDateTimeString()
	     << od_newline << od_endl;

    if ( execthr_ ) { execthr_->waitForFinish(); delete execthr_; }

    execthr_ = new Threads::Thread( mCB(this,CmdDriver,mkThread),
	BufferString( "CmdDriver file ", cmdfnm_ ).buf() );

    return true;
}


class SetCursorActivator: public Activator
{
public:
		SetCursorActivator( const MouseCursor::Shape& shape )
		    : actshape_( shape )			{}
    void	actCB( CallBacker* cb ) override
		{ uiCursorManager::setPriorityCursor( actshape_ ); }
protected:
    MouseCursor::Shape actshape_;
};


class UnsetCursorActivator: public Activator
{
public:
    void	actCB( CallBacker* cb ) override
		{ uiCursorManager::unsetPriorityCursor(); }
};


bool CmdDriver::waitForTimers()
{
    Threads::Locker lckr( cmddrvlock_ );
    const int sz = timerlist_.size();
    return sz;
}


void CmdDriver::prepareForResume()
{
    getModalStatus( curmodalstat_ );

    bool activitydetected = false;
    int nrtimesinactive = 0;

    while ( true )
    {
	if ( !activityStopped(false) )
	{
	    nrtimesinactive = 0;
	    if ( !activitydetected )
	    {
		activitydetected = true;
		if ( winstatetype_ == NoState )
		{
		    InteractSpec ispec( false );
		    ispec.dlgtitle_= 
			tr("Waiting for OpendTect to become inactive");
		    ispec.cancelbuttext_ = uiStrings::sHide();
		    interact( &ispec );
		}
	    }
	    mBusyWaitUntil( abort_ || activityStopped(false) );
	}

	nrtimesinactive++;
	if ( abort_ || nrtimesinactive==(activitydetected ? 10 : 5) )
	    break;

	Threads::sleep( 0.1 );
    }

    if ( activitydetected && winstatetype_==NoState )
	interact( 0 );
}


void CmdDriver::prepareForAbort()
{
    int nrtimesinactive = 0;

    while ( true )
    {
	if ( !activityStopped(false,false) )
	{
	    CallBack cb = mCB( this, CmdDriver, killTaskRunnerCB );
	    mActivateInGUIThread( cb, false );
	    nrtimesinactive = 0;
	}
	else
	    nrtimesinactive++;

	if ( nrtimesinactive == 10 )
	    return;

	Threads::sleep( 0.1 );
    }
}


void CmdDriver::pause( bool yn )
{
    if ( yn )
    {
	pause_ = true;
	resume_ = false;
    }
    else if ( pause_ )
	resume_ = true;
}


void CmdDriver::exitApplCB( CallBacker* )
{
    Threads::sleep( 0.3 );  // Assuming CmdDriver thread will sleep afterwards

    mLogStrm << od_endl;
    mTimeStrm << "EXIT" << od_endl;

    abort_ = true;
    applwin_ = 0;
}


void CmdDriver::mkThread( CallBacker* )
{
    reInit();
    wcm_->reInit();

    getNrActiveCmdDrivers( 1 );

    if ( applwin_ )
    {
	applwin_->activatedone.notify( mCB(this,CmdDriver,activateDone) );
	applwin_->windowClosed.notify( mCB(this,CmdDriver,exitApplCB) );
    }
    Timer::timerStarts()->notify( mCB(this,CmdDriver,timerStartsCB) );
    Timer::timerStopped()->notify( mCB(this,CmdDriver,timerStoppedCB) );
    Timer::timerShoots()->notify( mCB(this,CmdDriver,timerShootsCB) );
    Timer::timerShot()->notify( mCB(this,CmdDriver,timerShotCB) );

    winstack_.moveFrameToTop( applwin_ );

    SetCursorActivator cursorsetter( MouseCursor::Forbidden );
    CallBack cb = mCB( &cursorsetter, SetCursorActivator, actCB );
    mActivateInGUIThread( cb, true );

    prepareForResume();
    prevmodalstat_ = curmodalstat_;
    wildmodalsignatures_ = curmodalstat_.signatures_;

    actionidx_ = 0;
    bool prepareforabort = false;

    while ( !abort_ )
    {
	if ( pause_ && !openqdialog_ )
	{
	    storeModalStatus();

	    mTimeStrm << "PAUSE" << od_endl;
	    if ( interactspec_ && interactspec_->wait_ )
	    {
		const CallBack icb( mCB(this,CmdDriver,interactCB) );
		mActivateInGUIThread( icb, false );
	    }
	    else	// trigger CmdDriverDlg to set "Resume" button
	    {
		InteractSpec ispec( false );
		interact( &ispec );
	    }

	    while ( !resume_ && !abort_ )
	    {
		Threads::sleep( 0.1 );

		if ( !verifyWinState() )
		{
		    prepareForResume();
		    if ( !verifyWinState() )
		    {
			interact( 0 );
			pause( false );
		    }
		}
	    }

	    if ( winstatetype_ == NoState )
		prepareForResume();

	    storeModalStatus();

	    winstatewin_.setEmpty();
	    winstatetype_ = NoState;
	    pause_ = false;
	    resume_ = false;
	    wcm_->flush( true );
	}

	if ( abort_ || actionidx_>=actions_.size() )
	    break;

	Threads::Locker lckr( cmddrvlock_ );
	const char* actstr = actions_[actionidx_]->line_;
	lckr.unlockNow();

	if ( actionidx_ && actstr[0]=='[' )
	    mLogStrm << od_endl;

	mTimeStrm << "ACT:  " << actstr << od_endl;

	recoverystep_ = NoClue;
	prepareforabort = true;

	BufferString substactstr;
	const int nrsubst = idm_->substitute( actstr, substactstr );
	if ( nrsubst )
	    mTimeStrm << "$UB$: " << substactstr << od_endl;

	curactjumped_ = false;
	bool ok = false;

	if ( nrsubst < 0 )
	{
	    mParseErrStrm << "Failure at " << (-nrsubst) << " substitution"
			  << (nrsubst<-1 ? "s" : "") << od_endl;
	}
	else
	    ok = doAction( substactstr );

	if ( abort_ )
	    break;

	wcm_->flush( ok );

	if ( !ok )
	{
	    mTimeStrm << "FAIL" << od_endl;
	    failed_ = true;

	    if ( onError()==Recover && recover() )
		continue;

	    break;
	}
	else
	    mTimeStrm << "OK" << od_endl;

	if ( !curactjumped_ )
	    moveActionIdx( 1 );
    }

    if ( abort_ && prepareforabort )
	prepareForAbort();

    mLogStrm << od_newline;
    mTimeStrm << ( abort_ ? "ABORT" : "END" ) << od_newline << od_endl;

    uiMainWin::programActiveWindow( 0 );

    UnsetCursorActivator cursorunsetter;
    cb = mCB( &cursorunsetter, UnsetCursorActivator, actCB );
    mActivateInGUIThread( cb, true );

    Timer::timerStarts()->remove( mCB(this,CmdDriver,timerStartsCB) );
    Timer::timerStopped()->remove( mCB(this,CmdDriver,timerStoppedCB) );
    Timer::timerShoots()->remove( mCB(this,CmdDriver,timerShootsCB) );
    Timer::timerShot()->remove( mCB(this,CmdDriver,timerShotCB) );
    if ( applwin_ )
    {
	applwin_->windowClosed.remove( mCB(this,CmdDriver,exitApplCB) );
	applwin_->activatedone.remove( mCB(this,CmdDriver,activateDone) );
    }
    getNrActiveCmdDrivers( -1 );

    cb = mCB( this, CmdDriver, executeFinishedCB );
    mActivateInGUIThread( cb, false );
}


void CmdDriver::executeFinishedCB( CallBacker* )
{
    executeFinished.trigger();
}


void CmdDriver::killTaskRunnerCB( CallBacker* )
{
    mDynamicCastGet( uiTaskRunner*, uitaskrun, uiMainWin::activeModalWindow() );
    if ( uitaskrun )
	uitaskrun->reject( 0 );
}


void CmdDriver::setWait( float time, bool regular )
{
    if ( regular )
    {
	const float regtime = time<0.0 ? 0.0f : time;
	pendingwait_ += regtime - regularwait_;
	regularwait_ = regtime;
    }
    else
	pendingwait_ += time;
}


void CmdDriver::setSleep( float time, bool regular )
{
    if ( regular )
    {
	const float regtime = time<0.0 ? 0.0f : time;
	pendingsleep_ += regtime - regularsleep_;
	regularsleep_ = regtime;
    }
    else if ( time <= 0.0 )
	pendingsleep_ += time;
    else if ( pendingsleep_ >= 0.0 )
    {
	Threads::sleep( time );
	mBusyWaitUntil( activityStopped(false,false) );
    }
    else if ( pendingsleep_+time > 0.0 )
    {
	Threads::sleep( pendingsleep_+time );
	pendingsleep_ = 0.0;
	mBusyWaitUntil( activityStopped(false,false) );
    }
}


#define mCheckFuncProcExchange( nameword, token ) \
\
    BufferString funcprocname( nameword ); \
    char* parenthesisptr = funcprocname.find( '(' ); \
    if ( parenthesisptr ) \
    { \
	*parenthesisptr = '\0'; \
	const BufferString fackey = Function::factoryKey( funcprocname ); \
	PtrMan<Function> func = Function::factory().create( fackey, drv_ ); \
	FileMultiString keyfms; \
	keyfms += funcprocname; keyfms += "\a"; \
	if ( (!func)==idm_->doesExist(keyfms) && (!func)==(token=='?') ) \
	{ \
	    mParseErrStrm << "Expect '" << token << "'-operator in front of " \
			  << (token=='?' ? "user-defined procedure: " \
					 : "built-in function: ") \
			  << funcprocname << od_endl; \
	    return false; \
	} \
    }

#define mSpendPendingSleep() \
{ \
    if ( pendingsleep_ > 0 ) \
    { \
	Threads::sleep( pendingsleep_ ); \
	pendingsleep_ = 0.0; \
	mBusyWaitUntil( activityStopped(false,false) ); \
    } \
}

#define mAtCtrlFlowAction() \
    ( actionidx_<actions_.size() && actions_[actionidx_]->gotoidx_>=0 )

bool CmdDriver::doAction( const char* actstr )
{
    mSkipBlanks( actstr );
    BufferString firstword;
    const char* parstr = getNextNonBlanks( actstr, firstword.getCStr() );

    bool doubtwinassert = mAtCtrlFlowAction() && !mMatchCI(firstword, "Def")
					      && !mMatchCI(firstword, "Fed");

    BufferString altparstr( actstr );
    char* assignptr =
		(char *) StringProcessor(altparstr).findAssignment( "=~?(" );

    if ( firstword[0] == '[' )
    {
	firstword = "WinAssert";
	doubtwinassert = true;
	parstr = actstr;
    }
    else if ( firstword.contains('(') && *assignptr=='(' )
    {
	firstword = "Call";
	parstr = actstr;
	doubtwinassert = true;
    }
    else if ( assignptr && *assignptr!='(' )
    {
	BufferString otherargs = assignptr+1;

	if ( *assignptr == '?' )
	{
	    otherargs = getNextNonBlanks( assignptr+1, firstword.getCStr() );
	    if ( firstword.contains('(') )
	    {
		mCheckFuncProcExchange( firstword, '=' );
		firstword = "Call";
		otherargs = assignptr;
		doubtwinassert = true;
	    }
	    else if ( !firstword.isEmpty() &&
		      !Command::isQuestionName(firstword,*this) )
	    {
		mParseErrStrm << "Command is not a question: "
                                << firstword << od_endl;
		return false;
	    }
	}
	else if ( *assignptr == '~' )
	{
	    firstword = "Try";
	    doubtwinassert = true;
	}
	else if ( *assignptr == '=' )
	{
	    BufferString name;
	    getNextNonBlanks( assignptr+1, name.getCStr() );
	    mCheckFuncProcExchange( name, '?' );

	    if ( !idm_->doesExist(name) && Command::isQuestionName(name,*this) )
	    {
		mParseErrStrm << "Expect '?'-operator in front of "
			      << "question command: " << name << od_endl;
		return false;
	    }

	    firstword = "Assign";
	}

	*assignptr = '\0';
	altparstr += altparstr.isEmpty() ? "_dummyvar " : " ";
	altparstr += otherargs;
	parstr = altparstr;
    }

    mSkipBlanks( parstr );
    const BufferString fackey = Command::factoryKey( firstword );
    PtrMan<Command> cmd = Command::factory().create( fackey, drv_ );
    if ( !cmd )
    {
	mParseErrStrm << (firstword.isEmpty() ? "Missing command"
					      : "Command not recognized: ")
		      << firstword << od_endl;
	return false;
    }

    if ( !localsearchenv_ && tryoutstack_.isEmpty() )
    {
	if ( doubtwinassert )
	    winassertsafe_ = false;
	else if ( cmd->isVisualCommand() && !verifyWinAssert() )
	    return false;
    }

    if ( !cmd->isOpenQDlgCommand() && openqdialog_ )
    {
	mWinErrStrm << "Command not supported for open QDialog" << od_endl;
	return false;
    }

    if ( !cmd->isLocalEnvCommand() && localsearchenv_ )
    {
	mParseErrStrm << "Keystring accepting command required in local "
		      << "search environment" << od_endl;
	return false;
    }

    if ( cmd->isVisualCommand() )
	mSpendPendingSleep();

    uiobjchange_ = cmd->isUiObjChangeCommand();
    const bool res = cmd->act(parstr);
    uiobjchange_ = false;

    if ( !res )
	return false;

    if ( cmd->isVisualCommand() )
	pendingsleep_ += regularsleep_;

    return true;
}


bool CmdDriver::doLocalAction( uiObject* localenv, const char* actstr )
{
    localsearchenv_ = localenv;
    const bool res = doAction( actstr );
    localsearchenv_ = 0;
    return res;
}


bool CmdDriver::tryAction( const char* identname, const char* actstr )
{
    tryoutstack_.insertAt( identname, 0 );
    tryoutval_ = 1;
    const bool res = doAction( actstr );
    wcm_->flush( res );
    idm_->set( tryoutstack_[0], tryoutval_ );
    tryoutval_ = 1;
    tryoutstack_.removeSingle( 0 );
    return res;
}


const char* CmdDriver::curWinTitle( int aliasnr ) const
{
    return windowTitle( applwin_, (openqdialog_ ? 0 : curWin()), aliasnr );
}


bool CmdDriver::waitForProcessing()
{
    Threads::Locker lckr( cmddrvlock_ );
    const int curnrproc = activatorlist_.size() + timeoutlist_.size();
    lckr.unlockNow();

    const int prevmodalbalance = prevmodalstat_.nrmodalwins_ - prevnrproc_;
    const int curmodalbalance = curmodalstat_.nrmodalwins_ - curnrproc;

    if ( curmodalbalance == prevmodalbalance )
	return false;

    if ( wildmodalsignatures_.isSubsetOf(curmodalstat_.signatures_) )
	return true;

    if ( mIsUdf(wildmodalclosedstamp_) )
	wildmodalclosedstamp_ = Time::getMilliSeconds();

    if ( wildmodalclosedstamp_ >= 0 )
    {
	const float msecwait = mMAX( 300, pendingwait_*1000 );
	if ( Time::passedSince(wildmodalclosedstamp_) < msecwait )
	    return true;

	mWinWarnStrm << "No guarantee all processing stopped: add preceding "
		     << "Wait-command if more patience needed" << od_endl;

	wildmodalclosedstamp_ = -1;
    }
    return false;
}


bool CmdDriver::activityStopped( bool checkprocessing, bool checktimers )
{
    ModalStatus newmodalstat;
    getModalStatus( newmodalstat );

    if ( curmodalstat_ != newmodalstat )
    {
	curmodalstat_ = newmodalstat;
	if ( !checkprocessing )
	     return false;
    }

    if ( curmodalstat_.uitaskrunner_ )
	return false;

    const MouseCursor::Shape mcshape = uiCursorManager::overrideCursorShape();
    if ( mcshape==MouseCursor::Wait || mcshape==MouseCursor::Busy )
	return false;

    if ( checktimers && waitForTimers() )
	return false;

    if ( checkprocessing && waitForProcessing() )
	return false;

    openqdialog_ = curmodalstat_.activetype_ > uiMainWin::Main;
    return true;
}


void CmdDriver::activateDone( CallBacker* activator )
{
    Threads::Locker lckr( cmddrvlock_ );

    const int idx = activatorlist_.indexOf( activator );
    if ( idx >= 0 )
	delete activatorlist_.removeSingle( idx );
}


#define mTimerListUpdate( tmr ) \
{ \
    timerlist_ -= tmr; \
    if ( tmr->scriptPolicy()==Timer::KeepWaiting || \
	(tmr->scriptPolicy()==Timer::DefaultPolicy && tmr->isSingleShot()) ) \
	timerlist_ += tmr; \
}

void CmdDriver::timerStartsCB( CallBacker* cb )
{
    mDynamicCastGet( const Timer*, timer, cb );
    Threads::Locker lckr( cmddrvlock_ );
    mTimerListUpdate( timer );
}


void CmdDriver::timerStoppedCB( CallBacker* timer )
{
    Threads::Locker lckr( cmddrvlock_ );
    timerlist_ -= timer;
}


void CmdDriver::timerShootsCB( CallBacker* cb )
{
    mDynamicCastGet( const Timer*, timer, cb );
    Threads::Locker lckr( cmddrvlock_ );
    mTimerListUpdate( timer );

    if ( timerlist_.isPresent(timer) )
	timeoutlist_ += timer;

    if ( timer->isSingleShot() )
	timerlist_ -= timer;
}


void CmdDriver::timerShotCB( CallBacker* timer )
{
    Threads::Locker lckr( cmddrvlock_ );
    timeoutlist_ -= timer;
}


void CmdDriver::getModalStatus( ModalStatus& modalstat )
{
    ModalStatusActivator modalstatusinspector( modalstat );
    CallBack cb = mCB( &modalstatusinspector, ModalStatusActivator, actCB );
    mActivateInGUIThread( cb, true );
}


void CmdDriver::storeModalStatus()
{
    getModalStatus( curmodalstat_ );

    bool wildmodalclosed = false;

    for ( int idx=wildmodalsignatures_.size()-1; idx>=0; idx-- )
    {
	const char* wildmodalsig = wildmodalsignatures_[idx]->buf();
	if ( !curmodalstat_.signatures_.isPresent(wildmodalsig) )
	{
	    wildmodalsignatures_.removeSingle( idx );
	    wildmodalclosed = true;
	}
    }
    if ( wildmodalclosed || resume_ )
    {
	for ( int idx=0; idx<curmodalstat_.signatures_.size(); idx++ )
	{
	    const char* curmodalsig = curmodalstat_.signatures_[idx]->buf();
	    if ( !prevmodalstat_.signatures_.isPresent(curmodalsig) )
		wildmodalsignatures_.add( curmodalsig );
	}
    }
    prevmodalstat_ = curmodalstat_;
}


bool CmdDriver::prepareActivate( Activator* activator )
{
    if ( abort_ )
	return false;

    interceptstatus_ = NoInterception;
    uiMenu::addInterceptor( mCB(this,CmdDriver,dynamicMenuInterceptor) );

    storeModalStatus();

    Threads::Locker lckr( cmddrvlock_ );
    prevnrproc_ = activatorlist_.size() + timeoutlist_.size();
    activatorlist_ += activator;
    lckr.unlockNow();

    wildmodalclosedstamp_ = mUdf(int);

    uiMainWin::programActiveWindow( const_cast<uiMainWin*>(curWin()) );
    return true;
}


void CmdDriver::finishActivate()
{
    mBusyWaitUntil( abort_ || activityStopped() );

    pendingwait_ = regularwait_;

    uiMenu::removeInterceptor( mCB(this,CmdDriver,dynamicMenuInterceptor));
    if ( !interceptmenu_ && interceptstatus_!=NoInterception )
    {
	mWinWarnStrm << "Unexpected popup menu clicked away. Item selection "
		     << "needs \"Menu\"-variant of this command" << od_endl;
    }
    interceptmenu_ = false;
}


class DisplayToggleActivator: public Activator
{
public:
		DisplayToggleActivator( uiObject& dummy )
		    : actdummy_( dummy )			{}
    void	actCB( CallBacker* cb ) override
		{ actdummy_.display( !actdummy_.isDisplayed() ); }
protected:
    uiObject&	actdummy_;
};


void CmdDriver::forceQtToCatchUp()
{
    // Hack changing the display state of one (dummy) object in CmdController
    // window back and forth, while busy waiting for those tasks to be finished.
    // Afterwards, any previous task is considered to be finished as well.

    ObjectSet<uiMainWin> windowlist;
    uiMainWin::getTopLevelWindows( windowlist, false );
    for ( int idx=windowlist.size()-1; idx>=0; idx-- )
    {
	if ( !mMatchCI(windowlist[idx]->name(), controllerTitle()) )
	    windowlist.removeSingle( idx );
    }

    if ( windowlist.isEmpty() )
	return;

    ObjectSet<const CallBacker> objsfound;
    ObjectFinder objfinder( *windowlist[0] );
    objfinder.findNodes( ObjectFinder::CurWinTopGrp, &objsfound );

    for ( int idx=0; idx<objsfound.size(); idx++ )
    {
	const UIEntity uientity( objsfound[idx] );
	uiObject* dummy = const_cast<uiObject*>( uientity.object() );
	mDynamicCastGet( uiLabel*, uilbl, dummy );
	if ( uilbl && mSearchKey("").isMatching(uilbl->name()) )
	{
	    const bool olddispstate = dummy->isDisplayed();
	    DisplayToggleActivator displaytoggler( *dummy );
	    CallBack cb = mCB( &displaytoggler, DisplayToggleActivator, actCB );

	    mActivateInGUIThread( cb, true );
	    mBusyWaitUntil( dummy->visible()!=olddispstate );
	    mActivateInGUIThread( cb, true );
	    mBusyWaitUntil( dummy->visible()==olddispstate );
	    return;
	}
    }
}


void CmdDriver::waitForClearance()
{
    forceQtToCatchUp();
}


void CmdDriver::dynamicMenuInterceptor( CallBacker* cb )
{
    mDynamicCastGet( uiMenu*, popupmenu, cb );
    if ( !popupmenu )
	return;

    if ( !dispatchDynamicMenu(popupmenu) )
	popupmenu->doIntercept( true, 0 );
}


bool CmdDriver::dispatchDynamicMenu( uiMenu* popupmenu )
{
    interceptstatus_ = InterceptError;
    if ( !interceptmenu_ )
	return false;

    if ( interceptmode_ == Click )
    {
	mFindMenuItem( interceptmenupath_, *popupmenu, curitem );
	mParOnOffPre( "menu item", interceptonoff_, curitem->isChecked(),
		      curitem->isCheckable() );

	interceptstatus_ = InterceptReady;
	popupmenu->doIntercept( true, const_cast<uiAction*>(curitem) );
	return true;
    }

    const bool allowroot = interceptmode_ == NodeInfo;
    const MenuTracer mt( *popupmenu, *this );
    if ( mt.getMenuInfo(interceptmenupath_, allowroot, interceptmenuinfo_) )
	interceptstatus_ = InterceptReady;

    return false;
}


void CmdDriver::prepareIntercept( const FileMultiString& mnupath, int onoff,
				  InterceptMode mode )
{
    interceptmenu_ = true;
    interceptmenupath_ = mnupath;
    interceptonoff_ = onoff;
    interceptmode_ = mode;
}


bool CmdDriver::didInterceptSucceed( const char* objnm )
{
    if ( interceptstatus_ == InterceptReady )
	return true;

    if ( interceptstatus_ == NoInterception )
	mWinErrStrm << objnm << " did not popup a menu" << od_endl;

    return false;
}


const MenuInfo& CmdDriver::interceptedMenuInfo() const
{ return interceptmenuinfo_; }


bool CmdDriver::switchCurWin( const uiMainWin* newwin )
{
    return openqdialog_ ? !newwin : winstack_.moveToTop(newwin);
}


bool CmdDriver::verifyWinAssert( const char* newassertion )
{
    if ( newassertion )
    {
	winassertion_ = newassertion;
	winassertcs_ = casesensitive_;
	winassertsafe_ = true;
    }

    if ( winassertion_.isEmpty() )
	return true;

    BufferString winstr = winassertion_;
    mParDisambiguator( "window name", winstr, selnr );
    ObjectSet<uiMainWin> windowlist;
    SearchKey(winstr,winassertcs_).getMatchingWindows( applwin_, windowlist );
    mKeepSelection( windowlist, selnr );

    if ( windowlist.isEmpty() )
    {
	if ( !winassertsafe_ )
	    return true;

	mWinErrStrm << "No window matching assertion [" << winassertion_
		    << "]" << od_endl;
	recoverystep_ = NextAssertion;
	return false;
    }

    if ( windowlist.size() > 1 )
    {
	mWinWarnStrm << "Oldest one assumed from " << windowlist.size()
		     << " windows matching assertion: [" << winstr
		     << "]" << od_endl;
	winassertion_ += "#1";
    }

    if ( newassertion )
	SearchKey(winstr,winassertcs_).getMatchingWindows( applwin_,
							   windowlist, wcm_ );

    if ( switchCurWin(windowlist[0]) )
    {
	winassertsafe_ = true;
	return true;
    }

    if ( !winassertsafe_ )
	return true;

    mWinErrStrm << "Current window does not match assertion ["
		<< winassertion_ << "]" << od_endl;
    recoverystep_ = CloseCurWin;
    return false;
}


bool CmdDriver::verifyWinState( const char* newwinstr, WinStateType newwinstate)
{
    if ( newwinstr )
    {
	winstatewin_ = newwinstr;
	winstatetype_ = newwinstate;
    }

    if ( winstatewin_.isEmpty() )
	return true;

    BufferString winstr = winstatewin_;
    mParDisambiguator( "window name", winstr, selnr );
    ObjectSet<uiMainWin> windowlist;
    mSearchKey(winstr).getMatchingWindows( applwin_, windowlist );
    mKeepSelection( windowlist, selnr );

    if ( winstatetype_==Accessible || winstatetype_==Inaccessible )
    {
	const uiMainWin* oldcurwin = curWin();
	for ( int idx=windowlist.size()-1; idx>=0; idx-- )
	{
	    if ( !switchCurWin(windowlist[idx]) )
		windowlist.removeSingle( idx );
	}
	switchCurWin( oldcurwin );
    }

    if ( winstatetype_==Inexistent || winstatetype_==Inaccessible )
    {
	mSearchKey(winstr).getMatchingWindows( applwin_, windowlist, wcm_ );
	return windowlist.isEmpty();
    }

    if ( newwinstr )
	mSearchKey(winstr).getMatchingWindows( applwin_, windowlist, wcm_ );

    return !windowlist.isEmpty();
}


const char* CmdDriver::locateCmdMark( const char* actstr )
{
    mSkipBlanks( actstr );

    if ( *actstr == '[' )
	return actstr;

    const char* assignptr = StringProcessor(actstr).findAssignment( "=~?(" );

    if ( !assignptr )
	return actstr;

    if ( *assignptr == '~' )
	return locateCmdMark( assignptr+1 );

    BufferString firstword;
    getNextNonBlanks( actstr, firstword.getCStr() );
    if ( *assignptr=='(' && !firstword.contains('(') )
	return actstr;

    if ( *assignptr != '?' )
	return assignptr;

    actstr = assignptr + 1;
    mSkipBlanks( actstr );
    getNextNonBlanks( actstr, firstword.getCStr() );
    return firstword.contains('(') ? firstOcc(actstr,'(') : actstr;
}


bool CmdDriver::recover()
{
    if ( onError()==Stop || recoverystep_==NoClue )
	return false;

    if ( recoverystep_ == NextCmd )
    {
	mTimeStrm << "RECOVER: Command skipped" << od_endl;
	moveActionIdx( 1 );
	return true;
    }

    if ( !winassertsafe_ )
	return false;

    if ( recoverystep_ == CloseCurWin )
    {
	BufferString oldwintitle = curWinTitle();
	mParWinStrPre( oldwinlist, oldwintitle, 0, false );

	mSpendPendingSleep();

	if ( openqdialog_ )
	    mActivate( CloseQDlg, Activator(0) )
	else
	    mActivate( Close, Activator(*curWin()) );

	mTimeStrm << "RECOVER: Closing of window \""
		  << oldwintitle << "\" ";

	mParWinStrPre( newwinlist, oldwintitle, 0, false );
	if ( newwinlist.size() >= oldwinlist.size() )
	{
	    mLogStrm << "failed" << od_endl;
	    return false;
	}

	mLogStrm << "successful" << od_endl;
	pendingsleep_ += regularsleep_;
	return true;
    }

    if ( recoverystep_ == NextAssertion )
    {
	while ( actionidx_<actions_.size()-1 )
	{
	    moveActionIdx( 1 );
	    const char* cmdmark = locateCmdMark( actions_[actionidx_]->line_ );

	    if ( *cmdmark == '[' )
	    {
		mTimeStrm << "RECOVER: Jump to next window assertion"
			  << od_endl;
		return true;
	    }

	    BufferString firstword;
	    getNextNonBlanks( cmdmark, firstword.getCStr() );
	    if ( mMatchCI(firstword, "Def") )
		jump();
	    else if ( mAtCtrlFlowAction() || *cmdmark=='(' )
		break;
	}

	return false;
    }

    return false;
}


void CmdDriver::interactCB( CallBacker* )
{
    CBCapsule<const InteractSpec*> caps( interactspec_, this );
    interactRequest.trigger( &caps );
    interactspec_ = 0;
}


void CmdDriver::interact( const InteractSpec* ispec )
{
    mDefineStaticLocalObject( InteractSpec, interactbuf_, );

    interactspec_ = 0;
    if ( ispec )
    {
	interactbuf_ = *ispec;
	interactspec_ = &interactbuf_;
	if ( ispec->wait_ )
	{
	    pause();
	    return;
	}
    }

    mActivateInGUIThread( mCB(this,CmdDriver,interactCB), false );
}


void CmdDriver::moveActionIdx( int nrlines )
{
    actionidx_ += nrlines;
    lastmove_ = nrlines;

    if ( actionidx_>0 && actionidx_<=actions_.size() )
    {
	const int insertidx = actions_[actionidx_-1]->insertidx_;
	if ( insertidx >= 0 )
	{
	    const int sz = actionidx_ - insertidx;
	    for ( int idx=0; idx<sz; idx++ )
		delete actions_.removeSingle( insertidx );

	    actionidx_ -= sz;
	    winassertsafe_ = false;
	}
    }

    lastmove_ -= actionidx_;
    while ( actionidx_<actions_.size() && !curactjumped_ )
    {
	const char* cmdmark = locateCmdMark( actions_[actionidx_]->line_ );
	BufferString cmdname;
	getNextNonBlanks( cmdmark, cmdname.getCStr() );

	if ( mMatchCI(cmdname, "ElseIf") || mMatchCI(cmdname, "Else") )
	{
	    actionidx_ = actions_[actionidx_]->gotoidx_;
	    continue;
	}
	else if ( mMatchCI(cmdname, "Od") || mMatchCI(cmdname, "Rof") )
	    actionidx_ = actions_[actionidx_]->gotoidx_;

	break;
    }

    lastmove_ += actionidx_;
}


void CmdDriver::end()
{
    curactjumped_ = true;
    moveActionIdx( actions_.size()-actionidx_ );
}


void CmdDriver::jump( int extralines )
{
    if ( actionidx_<0 || actionidx_>=actions_.size() )
	return;

    if ( actions_[actionidx_]->gotoidx_ < 0 )
	return;

    curactjumped_ = true;
    moveActionIdx( actions_[actionidx_]->gotoidx_ - actionidx_ + extralines );
}


bool CmdDriver::streamBlocked( bool parse, const char* tag )
{
    const bool error = SearchKey("ERROR", false).isMatching( tag );
    const bool warn  = SearchKey("WARN",  false).isMatching( tag );

    const bool tryout = !tryoutstack_.isEmpty() && !parse;

    if ( tryout && error )
	tryoutval_ = 0;
    if ( tryout && warn && tryoutval_ )
	tryoutval_ = -1;

    if ( logmode_==LogBasic && warn )
	return true;

    return tryout && logmode_!=LogAll;
}


bool CmdDriver::insertProcedure( int defidx )
{
    if ( defidx<0 || defidx>=actions_.size() )
	return false;

    const char* cmdmark = locateCmdMark( actions_[defidx]->line_ );
    BufferString cmdname;
    getNextNonBlanks( cmdmark, cmdname.getCStr() );
    if ( !mMatchCI(cmdname, "Def") || actions_[defidx]->gotoidx_<0 )
	return false;

    const int offset = actionidx_ + 1 - defidx;

    Threads::Locker lckr( cmddrvlock_ );

    for ( int idx=actions_[defidx]->gotoidx_; idx>=defidx; idx-- )
    {
	actions_.insertAt( new Action(*actions_[idx]), actionidx_+1 );

	if ( actions_[actionidx_+1]->gotoidx_ >= 0 )
	     actions_[actionidx_+1]->gotoidx_ += offset;
    }
    actions_[actionidx_+1]->line_ = "Dummy";

    const int returnidx = actions_[defidx]->gotoidx_ + offset;
    actions_[returnidx]->line_ = "Return";
    actions_[returnidx]->insertidx_ = actionidx_ + 1;

    lckr.unlockNow();

    moveActionIdx( 1 );
    return true;
}


} // namespace CmdDrive
