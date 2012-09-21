#ifndef command_h
#define command_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 RCS:           $Id$
 ________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "factory.h"
#include "identifierman.h"
#include "objectfinder.h"
#include "uidialog.h"
#include "uimainwin.h"
#include "uitreeview.h"
#include "cmddriver.h"
#include "timefun.h"

class BufferString;
class uiMenuItemContainer;
class uiMenuItem;


namespace CmdDrive
{

class Activator;


mClass(uiCmdDriver) Command
{
public:

    mDefineFactory1ParamInClass( Command, CmdDriver&, factory );
    static void		initStandardCommands();
    static const char*	factoryKey(const char* name);

    			Command(CmdDriver& cmddrv)
			    : drv_(cmddrv)
    			{}

    virtual const char* name() const			= 0; 			
    virtual bool	act(const char* parstr)		= 0;

    virtual bool	isOpenQDlgCommand() const	{ return true; }
    virtual bool	isLocalEnvCommand() const	{ return false; }
    virtual bool	isVisualCommand() const		{ return true; }
    virtual bool	isUiObjChangeCommand() const	{ return false; }

    static bool		isQuestionName(const char*,CmdDriver&);

    enum FormTag	{ NoForm=0, Text, Number, Colour, Value, Angle,
			  Minimum, Maximum, Step, Percentage, FilePath };

protected:

    static const char*	createFactoryKey(const char* keyword);

    CmdDriver&		drv_;
    uiMainWin*		applWin();

    const char*		outputDir() const;

    bool		switchCurWin(uiMainWin*);
    const uiMainWin*	curWin() const;
    bool		openQDlg() const;

    CmdDriver::OnErrorTag onError() const;

    void		setOnError(CmdDriver::OnErrorTag);
    bool		verifyWinAssert(const char* newwinstr=0);
    bool		verifyWinState(const char* newwinstr=0,
				       WinStateType newwinstate=NoState);

    void		setRecoveryStep(CmdDriver::RecoveryTag);

    void		setCaseSensitive(bool yn);
    bool		isCaseSensitive() const;
    void		skipGreyOuts(bool yn=true);
    bool		greyOutsSkipped() const;
    bool		goingToChangeUiObj() const;

    void		setSleep(float time,bool regular);
    void		setWait(float time,bool regular);

    const uiObject*	localSearchEnv() const;
    bool		doLocalAction(uiObject* localenv,const char* actstr);
    bool		tryAction(const char* identname,const char* actstr);

    bool		prepareActivate(Activator*);
    void		finishActivate();
    void		waitForClearance();

    void		prepareIntercept(const FileMultiString& menupath,
				    int onoff,
				    CmdDriver::InterceptMode=CmdDriver::Click);

    bool		didInterceptSucceed(const char* objnm);
    const MenuInfo&	interceptedMenuInfo() const;

    void		interact(const InteractSpec*);

    WildcardManager&	wildcardMan();
    IdentifierManager&	identifierMan();
    ExprInterpreter&	exprInterpreter();

    void		end();
    void		jump(int extralines=0);                    
    int			lastActionIdxMove() const;

    int			curActionIdx() const;
    bool		insertProcedure(int defidx);
};


#define mStartDeclCmdClassNoActNoEntry(cmdkey,parentclass) \
\
mClass(uiCmdDriver) cmdkey##Cmd : public parentclass \
{ \
public: \
    			cmdkey##Cmd(CmdDriver& cmddrv) \
			    : parentclass(cmddrv) \
			{} \
\
    static const char*	keyWord()			{ return #cmdkey; } \
    virtual const char* name() const			{ return keyWord(); }

#define mStartDeclCmdClassNoAct(cmdkey,parentclass) \
\
    mStartDeclCmdClassNoActNoEntry(cmdkey,parentclass) \
\
    static Command*	createInstance(CmdDriver& cmddrv) \
			{ return new cmdkey##Cmd(cmddrv); } \
    static void		initClass() \
			{ factory().addCreator( createInstance, \
					        createFactoryKey(keyWord()) ); }

#define mStartDeclCmdClassNoEntry(cmdkey,parentclass) \
    mStartDeclCmdClassNoActNoEntry(cmdkey,parentclass) \
    virtual bool	act(const char* parstr);

#define mStartDeclCmdClass(cmdkey,parentclass) \
    mStartDeclCmdClassNoAct(cmdkey,parentclass) \
    virtual bool	act(const char* parstr);

#define mEndDeclCmdClass \
};


mStartDeclCmdClassNoActNoEntry( UiObject, Command )
    virtual bool	isOpenQDlgCommand() const	{ return false; }
    virtual bool	isLocalEnvCommand() const	{ return true; }
    virtual bool	isUiObjChangeCommand() const	{ return true; }
mEndDeclCmdClass

mStartDeclCmdClassNoActNoEntry( UiObjQuestion, Command )
    virtual bool	isOpenQDlgCommand() const	{ return false; }
    virtual bool	isLocalEnvCommand() const	{ return true; }
    virtual bool	isVisualCommand() const		{ return false; }
mEndDeclCmdClass

mStartDeclCmdClassNoActNoEntry( Stealth, Command )
    virtual bool	isVisualCommand() const		{ return false; }
mEndDeclCmdClass


//====== Activator ==========================================================

/*  The Activator base class handles the GUI thread callback after calling the
    uiMainWin::activateInGUIThread( mCB(activator,Activator,actCB), busywait )
    function. Its actCB(cber) function contains the code that must be executed
    in the GUI thread to prevent Qt from crashing.
   
    Apart from pointers or references to ui-objects, the Activator subclasses
    should be careful declaring pointer or reference data members. Copying is
    preferable, since their initialization by local variables will be unsafe
    in case activateInGUIThread(.,.) is going to be called with busywait=false.
*/

mClass(uiCmdDriver) Activator : public CallBacker
{
public:
    virtual		~Activator()			{};
    virtual void	actCB(CallBacker*)		= 0;
};


mClass(uiCmdDriver) CloseActivator: public Activator
{
public:
			CloseActivator(const uiMainWin& uimw)
			    : actmainwin_( const_cast<uiMainWin&>(uimw) )
			{} 
    void		actCB(CallBacker* cb)
			{ actmainwin_.close(); }
protected:
    uiMainWin&		actmainwin_;
};


mClass(uiCmdDriver) CloseQDlgActivator: public Activator
{
public:
			CloseQDlgActivator(int retval)
			    : actretval_( retval )
			{} 
    void		actCB(CallBacker* cb)
			{ uiMainWin::closeActiveModalQDlg(actretval_); }
protected:
    int			actretval_;
};


//====== Menu tracer ==========================================================


mClass(uiCmdDriver) MenuTracer
{
public:
    			MenuTracer(const uiMenuItemContainer& mnu,
				   CmdDriver& cmddrv)
			    : startmenu_(mnu), drv_(cmddrv)
			{}

    bool		findItem(const FileMultiString& menupath,
			    const uiMenuItem*& curitem,int* curitmidx=0) const;
    bool		getMenuInfo(const FileMultiString& menupath,
				    bool allowroot,MenuInfo&) const;

protected:

    CmdDriver&			drv_;
    const uiMenuItemContainer&  startmenu_;

    int			nrItems(const FileMultiString& menupath) const;
    bool		greyOutsSkipped() const;
    bool		goingToChangeUiObj() const;
};


//====== Parsing macros =======================================================

#define mParDQuoted( argnm, parstr, parnext, argstr, emptycheck, optional ) \
\
    BufferString argstr; \
    const char* parnext = StringProcessor(parstr).parseDQuoted( argstr ); \
    if ( (!optional && !parnext) || (emptycheck && parnext && !*argstr) ) \
    { \
	mParseErrStrm << (parnext ? "Empty " : "No ") << argnm \
		      << " specified" << std::endl; \
	return false; \
    } \
    if ( !parnext ) \
	parnext = parstr;


#define mParDisambiguatorRet( argnm, str, selnr, retfld ) \
    int selnr = StringProcessor(str).removeNumAppendix(); \
    if ( mIsUdf(selnr) ) \
    { \
	mParseErrStrm << "Non-zero integer required to disambiguate " \
		      << argnm << ": \"" << str << "\"" << std::endl; \
	return retfld; \
    } 

#define mParDisambiguator( argnm, str, selnr ) \
    mParDisambiguatorRet( argnm, str, selnr, false )


#define mParStrErrRet( objnm,nrfound,nrgrey,str,selnr,strnm,ambicheck,retfld ) \
\
    const int overflow = (!selnr ? 1 : abs(selnr)) - nrfound; \
\
    if ( overflow>0 || (!selnr && ambicheck && nrfound>1) ) \
    { \
	if ( nrfound && overflow>0 ) \
	    mWinErrStrm << "Impossible to select " << objnm << ": #" << selnr \
			<< std::endl; \
\
	BufferString dispstr = str; \
	replaceString( dispstr.buf(), "\a", "*" ); \
	mWinErrStrm << "Found " << nrfound \
		    << (greyOutsSkipped() ? " enabled " : " ") << objnm \
		    << "(s) defined by " << strnm << ": \"" << dispstr \
		    << "\"" << std::endl; \
\
	if ( greyOutsSkipped() && overflow>0 && overflow<=nrgrey ) \
	    mWinWarnStrm << "Did find " << nrgrey << " disabled " << objnm \
			 << "(s) defined by " << strnm << ": \"" << dispstr \
			 << "\"" << std::endl; \
\
	return retfld; \
    }

#define mKeepSelection( objsfound, selnr ) \
{ \
    if ( selnr ) \
    { \
	const int selidx = selnr>0 ? selnr-1 : selnr+objsfound.size(); \
	for ( int idx=objsfound.size()-1; idx>=0; idx-- ) \
	{ \
	    if ( idx != selidx ) \
		objsfound.remove( idx );  \
	} \
    } \
}

#define mParStrPreRet(objnm,objsfound,nrgrey,str,selnr,strnm,ambicheck,retfld) \
{ \
    const int nrfound = objsfound.size(); \
    mParStrErrRet( objnm,nrfound,nrgrey,str,selnr,strnm,ambicheck,retfld ) \
    mKeepSelection( objsfound, selnr ); \
} \

#define mParStrPre( objnm, objsfound, nrgrey, str, selnr, strnm, ambicheck ) \
    mParStrPreRet(objnm, objsfound, nrgrey, str, selnr, strnm, ambicheck, false)


#define mDisabilityCheck( objnm, nrobjs, disabled ) \
\
    if ( goingToChangeUiObj() && (disabled) ) \
    { \
	mWinErrStrm << (nrobjs>1 ? "Some s" : "S") << "elected " \
		    << objnm << (nrobjs>1 ? "s are" : " is") \
		    << " disabled for manipulation" << std::endl; \
	return false; \
    }


#define mParKeyStrInit( objnm, parstr, parnext, keys, selnr ) \
\
    mParDQuoted(objnm " keystring", parstr, parnext, keys##str, false, false); \
    mParDisambiguator( objnm " keystring", keys##str, selnr ); \
    mGetEscConvertedFMS( keys, keys##str, false ); \

#define mParKeyStrPre( objnm, objsfound, nrgrey, keys, selnr ) \
    mParStrPre( objnm, objsfound, nrgrey, keys.buf(), selnr, "key(s)", true ); \
    mDisabilityCheck( objnm, 1, !objsfound[0]->sensitive() ); \
    ObjectFinder wcmobjfinder( *curWin(), isCaseSensitive(), &wildcardMan() ); \
    wcmobjfinder.selectNodes( objsfound, keys );

#define mParOptPathStrInit( objnm, parstr, parnext, path, optional ) \
    mParDQuoted( objnm " path", parstr, parnext, path##str, false, optional ); \
    mGetEscConvertedFMS( path, path##str, false ); 

#define mParPathStrInit( objnm, parstr, parnext, path ) \
    mParOptPathStrInit( objnm, parstr, parnext, path, false ) \

#define mParWinStrInit( objnm, parstr, parnext, winstr, selnr, optional ) \
    mParDQuoted( objnm " name", parstr, parnext, winstr, true, optional ); \
    mParDisambiguator( objnm " name", winstr, selnr );


#define mParWinStrPre( windowlist, winstr, selnr, errorcheck ) \
\
    ObjectSet<uiMainWin> windowlist; \
    mSearchKey(winstr).getMatchingWindows( applWin(), windowlist ); \
    if ( errorcheck ) \
    { \
	mParStrPre( "window", windowlist, 0, winstr, selnr, "string", true ) \
	mSearchKey(winstr).getMatchingWindows( applWin(), windowlist, \
					       &wildcardMan() ); \
    } \
    else \
	mKeepSelection( windowlist, selnr );
    
// no selection at all: itemnr = mUdf(int)
// No selection number: itemnr = 0
// No item name: itemstr = "\a"

#define mParItemSelInit( objnm, parstr, parnext, itemstr, itemnr, optional ) \
\
    mParDQuoted( BufferString(objnm," name"), parstr, parnext, itemstr, \
		 false, true ); \
    mParDisambiguator( BufferString(objnm," name"), itemstr, itemnr ); \
    if ( parnext == parstr ) \
    { \
	const int num = strtol( parstr, const_cast<char**>(&parnext), 0 ); \
\
	if ( num || (optional && parnext==parstr) ) \
	{ \
	    itemnr = !num ? mUdf(int) : num; \
	    itemstr = "\a"; \
	} \
	else \
	{ \
	    mParseErrStrm << "Name or non-zero integer needed to select " \
			  << objnm << std::endl; \
	    return false; \
	} \
    }


/* Internal onoff variables: on=1, off=-1, unspecified/unswitchable=0

   Beware this differs from the identifier value returned by any
   'Is...On'-question command in scripts! (off=0, unswitchable=-1)
*/

#define mParOnOffInit( parstr, parnext, onoff ) \
\
    BufferString onoffword; \
    const char* parnext = getNextWord( parstr, onoffword.buf() ); \
    mSkipBlanks( parnext ); \
\
    int onoff = 0; \
    if ( onoffword=="1" || mMatchCI(onoffword,"On") ) \
	onoff = 1; \
    if ( onoffword=="0" || mMatchCI(onoffword,"Off") ) \
	onoff = -1; \
    if ( !onoff ) \
	parnext = parstr; \

#define mParOnOffPre( objnm, onoff, ischecked, checkable ) \
{ \
    if ( onoff && !(checkable) ) \
    { \
	mWinWarnStrm << "This " << objnm << " has no on/off switch" \
		     << std::endl; \
	onoff = 0; \
    } \
\
    if ( onoff == ((ischecked) ? 1 : -1) ) \
    { \
	mWinErrStrm << "This " << objnm << " was switched " \
		    << (onoff==1 ? "on" : "off") << " already" << std::endl; \
	setRecoveryStep( CmdDriver::NextCmd ); \
	return false; \
    } \
}

#define mParOnOffPost( objnm, onoff, ischecked ) \
{ \
    if ( onoff == ((ischecked) ? -1 : 1) ) \
    { \
	mWinWarnStrm << "Switching " << (onoff==1 ? "on" : "off") \
		     << " this " << objnm << " has been overruled" \
		     << std::endl; \
    } \
}


#define mParInputStr( objnm, parstr, parnext, inpstr, optional ) \
\
    BufferString filepathword; \
    const char* extraparstr = getNextWord( parstr, filepathword.buf() ); \
    mSkipBlanks( extraparstr ); \
    if ( !mMatchCI(filepathword,"FilePath") ) \
        extraparstr = parstr; \
\
    mParDQuoted( objnm, extraparstr, parnext, inpbufstr, false, true ); \
    if ( parnext == extraparstr ) \
    { \
	if ( extraparstr != parstr ) \
	{ \
	    mParseErrStrm << "FilePath-option expects double-quoted input " \
			  << "string" << std::endl; \
	    return false; \
	} \
	const double inpnum = strtod( parstr, const_cast<char**>(&parnext) ); \
        inpbufstr = toString( inpnum ); \
    } \
    else \
    { \
	if ( extraparstr != parstr ) \
	    StringProcessor(inpbufstr).makeDirSepIndep(); \
\
	StringProcessor(inpbufstr).removeCmdFileEscapes(); \
    } \
\
    const char* inpstr = parnext==parstr ? 0 : inpbufstr.buf(); \
    if ( !optional && !inpstr ) \
    { \
	mParseErrStrm << "Double-quoted string or numeric argument " \
		      << "expected as input" << std::endl; \
	return false; \
    }


#define mParSteps( parstr, parnext, nrsteps, minval, defval ) \
\
    char* parnext; \
    int nrsteps = strtol( parstr, &parnext, 0 ); \
    if ( parnext!=parstr && nrsteps<minval ) \
    { \
	mParseWarnStrm << "Number of steps should be at least " << minval \
		       << std::endl; \
    } \
    if ( parnext==parstr || nrsteps<minval ) \
	nrsteps = defval;


#define mPopUpWinInLoopCheck( prevwin ) \
\
    if ( openQDlg() || curWin()!=prevwin ) \
    { \
	mWinErrStrm << "Next step blocked by popped-up modal window" \
		    << std::endl; \
	return false; \
    }


#define mMatchMouseTag( tag, mousetagptr, clicktags ) \
\
    if ( matchStringCI(tag, mousetagptr) ) \
    { \
	    clicktags.add( tag ); \
	    mousetagptr += strlen( tag ); \
    }

#define mParMouse( parstr, parnext, clicktags, defaulttag ) \
\
    mSkipBlanks( parstr ); \
    BufferStringSet clicktags; \
    const char* parnext = parstr; \
    mMatchMouseTag( "Ctrl", parnext, clicktags ); \
    mMatchMouseTag( "Double", parnext, clicktags ); \
\
    mMatchMouseTag( "Left", parnext, clicktags ) \
    else mMatchMouseTag( "Right", parnext, clicktags ) \
    else \
	clicktags.add( clicktags.isEmpty() ? defaulttag : "Left" ); \
\
    if ( *parnext && !isspace(*parnext) ) \
	parnext = parstr; \


#define mButtonCmdMouseTagCheck( clicktags ) \
\
    if ( clicktags.isPresent("Right") || clicktags.isPresent("Double") ) \
    { \
	mParseWarnStrm << "Double or Right mouse-click has no (lasting) " \
		       << "effect on check-box" << std::endl; \
    }	


#define mParExpr( isarg, identnm, parstr, parnext, valstr, prescan ) \
\
    BufferString valstr; \
    const char* parnext = exprInterpreter().process( parstr, valstr, isarg ); \
\
    if ( !parnext || (!(prescan) && *exprInterpreter().errMsg()) ) \
    { \
	mTimeStrm << "EVAL: " << exprInterpreter().breakPrefix(); \
	mLogStrm << " ..." << std::endl; \
	if ( exprInterpreter().isParseError() ) \
	{ \
	    mParseErrStrm << exprInterpreter().errMsg() << std::endl; \
	} \
	else \
	    mWinErrStrm << exprInterpreter().errMsg() << std::endl; \
	\
	return false; \
    } \
\
    if ( !(prescan) && ((isarg) || !exprInterpreter().isResultTrivial()) ) \
    { \
	mTimeStrm << "EVAL: " << exprInterpreter().parsedExpr(); \
	StringProcessor strproc( valstr ); \
	const char* quote = strproc.convertToDouble() ? "" : "\""; \
	mLogStrm << " -->> " << identnm << (isarg ? "'" : "") \
		 << (strcmp(identnm,"") ? " = " : "") \
		 << quote << valstr << quote << std::endl; \
    }

#define mParIdentInit( parstr, parnext, identnm, allowdummy ) \
\
    BufferString firstarg, identnm; \
    const char* parnext = getNextWord( parstr, firstarg.buf() ); \
    if ( firstarg != "_dummyvar" ) \
	parnext = StringProcessor(parstr).parseIdentifier( identnm ); \
    else if ( allowdummy ) \
	identnm = firstarg; \
    else \
    { \
	mParseErrStrm << "Missing identifier" << std::endl; \
	return false; \
    } \
\
    if ( parnext && *parnext == '[' ) \
    { \
	mParseErrStrm << "If an array variable a[i] was intended, " \
		      << "use index substitution a_$i$ instead" << std::endl; \
	return false; \
    } \
    if ( !parnext || firstarg!=identnm ) \
    { \
	mParseErrStrm << "Invalid identifier: " << firstarg << std::endl; \
	return false; \
    } \
    if ( identifierMan().isPredefined(identnm) ) \
    { \
	mParseWarnStrm << "Reassigning a predefined identifier: " \
		       << identnm << std::endl; \
    }

#define mParEscIdentPost( identnm, val, args, addesc ) \
{ \
    mSkipBlanks( args ); \
    identifierMan().set( identnm, val ); \
    BufferString valstr = identifierMan().getValue( identnm ); \
    StringProcessor strproc( valstr ); \
    if ( addesc ) \
	strproc.addCmdFileEscapes(StringProcessor::sAllEscSymbols()); \
    \
    identifierMan().set( identnm, strproc.buf() ); \
    const char* quote = strproc.convertToDouble() ? "" : "\""; \
    mTimeStrm << "Q&A:  " << name() << (*args ? " " : "") << args \
	      << " -->> " << identnm << " = " <<  quote << strproc.buf() \
	      << quote << std::endl; \
}

#define mParIdentPost( identnm, val, args ) \
    mParEscIdentPost( identnm, val, args, true )


#define mParCase( parstr, parnext, casesensitive, optional ) \
\
    BufferString argword; \
    const char* parnext = getNextWord( parstr, argword.buf() ); \
    bool casesensitive = false; \
\
    if ( mMatchCI(argword,"Sensitive") ) \
	casesensitive = true; \
    else if ( !mMatchCI(argword,"Insensitive") ) \
    { \
	if ( !optional ) \
	{ \
	    mParseErrStrm << "Case-argument not in {Sensitive, Insensitive}" \
			  << std::endl; \
	    return false; \
	} \
	parnext = parstr; \
    }

#define mParExtraFormInit( parstr, parnext, form, extrastr ) \
\
    BufferString formword; \
    FileMultiString fms( extrastr ); \
    const char* parnext = getNextWord( parstr, formword.buf() ); \
    FormTag form = NoForm; \
    if ( mMatchCI(formword,"Text") ) \
	form = Text; \
    if ( mMatchCI(formword,"Number") && fms.indexOf("Number")>=0 ) \
	form = Number; \
    if ( mMatchCI(formword,"Color") && fms.indexOf("Color")>=0 ) \
	form = Colour; \
    if ( mMatchCI(formword,"Value") && fms.indexOf("Value")>=0 ) \
	form = Value; \
    if ( mMatchCI(formword,"Angle") && fms.indexOf("Angle")>=0 ) \
	form = Angle; \
    if ( mMatchCI(formword,"Minimum") && fms.indexOf("Minimum")>=0 ) \
	form = Minimum; \
    if ( mMatchCI(formword,"Maximum") && fms.indexOf("Maximum")>=0 ) \
	form = Maximum; \
    if ( mMatchCI(formword,"Step") && fms.indexOf("Step")>=0 ) \
	form = Step; \
    if ( mMatchCI(formword,"Percentage") && fms.indexOf("Percentage")>=0 ) \
	form = Percentage; \
    if ( mMatchCI(formword,"FilePath") && fms.indexOf("FilePath")>=0 ) \
	form = FilePath; \
    if ( form == NoForm ) \
	parnext = parstr;

#define mParFormInit( parstr, parnext, form ) \
    mParExtraFormInit( parstr, parnext, form, "Number" )

#define mParForm( answer, form, text, other ) \
\
    BufferString answer; \
    if ( form==NoForm || form==Text ) \
    { \
	answer = text; \
	StringProcessor(answer).cleanUp(); \
    } \
    else \
	answer = other;

#define mParExtraForm( answer, form, extratag, extra ) \
\
    if ( form == extratag ) \
	answer = extra;


#define mParFramed( parstr, parnext, framed ) \
\
    BufferString frameword; \
    const char* parnext = getNextWord( parstr, frameword.buf() ); \
    bool  framed = true; \
    if ( mMatchCI(frameword,"Selected") ) \
	framed = false; \
    else if ( !mMatchCI(frameword,"Framed") ) \
	parnext = parstr;


#define mParTail( partail ) \
\
    mSkipBlanks( partail ); \
    if ( (partail) && *(partail) ) \
    { \
	mParseErrStrm << "Command line ends with unexpected argument(s): " \
		      << (partail) << std::endl; \
	return false; \
    }


//====== MenuTracer macros ==================================================

#define mFindMenuItem( menupath, startmenu, curitem ) \
\
    const uiMenuItem* curitem; \
    if ( !MenuTracer(startmenu,drv_).findItem(menupath,curitem) ) \
	return false;

#define mGetMenuInfo( menupath, allowroot, startmenu, mnuinfo ) \
\
    MenuInfo mnuinfo; \
    if ( !MenuTracer(startmenu,drv_).getMenuInfo(menupath,allowroot,mnuinfo) ) \
	return false;


//====== ObjectFinder macros ==================================================

#define mFindObjs3Base( objsfound, objcls1, objcls2, objcls3, keys, warn ) \
\
    ObjectSet<const uiObject> objsfound; \
{ \
    ObjectFinder objfinder( *curWin(), isCaseSensitive() ); \
    if ( localSearchEnv() ) \
	objfinder.findNodes( localSearchEnv(), &objsfound ); \
    else \
	objfinder.findNodes( ObjectFinder::Everything, &objsfound ); \
\
    for ( int idx=objsfound.size()-1; idx>=0; idx-- ) \
    { \
	mDynamicCastGet( const objcls1*, uiobj1, objsfound[idx] ); \
	mDynamicCastGet( const objcls2*, uiobj2, objsfound[idx] ); \
	mDynamicCastGet( const objcls3*, uiobj3, objsfound[idx] ); \
	if ( !uiobj1 && !uiobj2 && !uiobj3 ) \
	    objsfound.remove( idx ); \
    } \
\
    int errkeyidx; \
    if ( !objfinder.selectNodes(objsfound, keys, &errkeyidx) && warn ) \
    { \
	mWinWarnStrm << "No object with key \"" << keys[errkeyidx] << "\" in " \
		     << ( localSearchEnv() ? "local search environment" \
					  : "current window" ) << std::endl; \
    } \
}

#define mFindObjs2Base( objsfound, objcls1, objcls2, keys, warn ) \
    mFindObjs3Base( objsfound, objcls1, objcls1, objcls2, keys, warn ) 

#define mFindObjsBase( objsfound, objclass, keys ) \
    mFindObjs2Base( objsfound, objclass, objclass, keys, true )

#define mFindObjects3( objsfound, objcls1, objcls2, objcls3, keys, nrgrey ) \
    mFindObjs3Base( objsfound, objcls1, objcls2, objcls3, keys, true ); \
    const int nrgrey = ObjectFinder::deleteGreys(objsfound, greyOutsSkipped());

#define mFindObjects2( objsfound, objcls1, objcls2, keys, nrgrey ) \
    mFindObjs2Base( objsfound, objcls1, objcls2, keys, true ); \
    const int nrgrey = ObjectFinder::deleteGreys(objsfound, greyOutsSkipped());

#define mFindObjects( objsfound, objclass, keys, nrgrey ) \
    mFindObjsBase( objsfound, objclass, keys ); \
    const int nrgrey = ObjectFinder::deleteGreys(objsfound, greyOutsSkipped());

#define mFindListTableObjs( objnm, objsfound, objclass, keys, nrgrey ) \
\
    mFindObjsBase( objsfound, objclass, keys ); \
    mFindObjs2Base( objsfound2, objclass, uiTreeView, keys, false ); \
    const bool uilviewonly = objsfound.isEmpty() && !objsfound2.isEmpty(); \
    const bool uilviewcloser = !objsfound.isEmpty() && \
			       objsfound2.indexOf(objsfound[0])<0; \
    if ( uilviewonly || uilviewcloser ) \
    { \
	mWinWarnStrm << "Skipped " << objsfound2.size() << " tree(s) " \
		     << (uilviewcloser ? "more closely" : "") \
		     << " defined by key(s): \"" << keys.buf() \
		     << "\". Possibly resembling a " << objnm \
		     << ", but requiring a \"Tree\"-command" << std::endl; \
    } \
    const int nrgrey = ObjectFinder::deleteGreys(objsfound, greyOutsSkipped());


//====== Activate macros ======================================================

#define mActivateInGUIThread( cb, busywait ) \
{ \
    uiMainWin* applwin = applWin(); \
    if ( applwin ) \
	applwin->activateInGUIThread( cb, busywait ); \
}


#define mActInGUIThread( typ, constructorcall, waitclear ) \
{ \
    typ##Activator* activator = new typ##constructorcall; \
    if ( prepareActivate(activator) ) \
    { \
	CallBack cb = mCB( activator, typ##Activator, actCB ); \
	mActivateInGUIThread( cb, false ); \
	finishActivate(); \
\
	if ( waitclear ) \
	    waitForClearance(); \
    } \
    else \
	delete activator; \
}


#define mActivate( acttyp, constructorcall ) \
    mActInGUIThread( acttyp, constructorcall, true )

#define mActivateNoClearance( acttyp, constructorcall ) \
    mActInGUIThread( acttyp, constructorcall, false )


}; // namespace CmdDrive

#endif

