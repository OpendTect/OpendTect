/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          February 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: drivercommands.cc,v 1.1 2012-09-17 12:37:41 cvsjaap Exp $";

#include "drivercommands.h"

#include "cmddriverbasics.h"
#include "interpretexpr.h"

#include "mousecursor.h"


namespace CmdDrive
{


bool WinAssertCmd::act( const char* parstr )
{
    BufferString winassertion;
    const char* partail = StringProcessor(parstr).parseBracketed(winassertion); 
    if ( !partail )
    {
	mParseErrStrm << "Missing right bracket in window assertion"
		      << std::endl;
	return false;
    }
    BufferString dummywinstr = winassertion;
    mParDisambiguator( "window name", dummywinstr, dummyselnr );
    mParTail( partail );

    return verifyWinAssert( winassertion );
}


bool WindowCmd::act( const char* parstr )
{
    mParWinStrInit( "window", parstr, partail, winstr, selnr, false );
    mParTail( partail );
    mParWinStrPre( windowlist, winstr, selnr, true ); 

    if ( switchCurWin(windowlist[0]) )
	return true;

    mWinErrStrm << "Cannot switch to window that has modal child"
		<< std::endl;
    return false;
}


bool IsWindowCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParDQuoted( "window", parnext, parnexxt, winstr, true, true );
    const bool defaultwin = parnext==parnexxt;

    BufferString wintitle = winstr;
    mParDisambiguator( "window name", wintitle, selnr );

    BufferString winprop; 
    const char* partail = getNextWord( parnexxt, winprop.buf() ); 

    BufferString answer;
    if ( winprop.isEmpty() || mMatchCI(winprop, "Existent") ||
			      mMatchCI(winprop, "Accessible") )
    {
	const WinStateType winstatetyp =
		    mMatchCI(winprop, "Accessible") ? Accessible : Existent;

	answer = defaultwin || verifyWinState(winstr,winstatetyp) ? 1 : 0;
	verifyWinState( "" );
    }
    else
    {
	mParWinStrPre( windowlist, wintitle, selnr, !defaultwin );
	const uiMainWin* uimw = openQDlg() ? 0 : curWin();
	if ( !defaultwin )
	   uimw = windowlist[0];

	if ( mMatchCI(winprop, "Normal") )
	{
	    answer = uimw &&
		     (uimw->isMinimized() || uimw->isMaximized()) ? 0 : 1;
	}
	else if ( mMatchCI(winprop, "Maximized") )
	    answer = uimw && uimw->isMaximized() ? 1 : 0;
	else if ( mMatchCI(winprop, "Minimized") )
	    answer = uimw && uimw->isMinimized() ? 1 : 0;
	else if ( mMatchCI(winprop, "Modal") )
	    answer = !uimw || uimw->isModal() ? 1 : 0;
	else if ( mMatchCI(winprop, "QDialog") )
	    answer = uimw ? 0 : 1;
	else
	    partail = parnexxt;
    }

    mParTail( partail );
    mParIdentPost( identname, answer, parnext );
    return true;
}


bool SleepCmd::act( const char* parstr )
{
    char* parnext;
    double time = strtod( parstr, &parnext );

    BufferString sleeptype; 
    const char* partail = getNextWord( parnext, sleeptype.buf() ); 
    bool regular = false;

    if ( mMatchCI(sleeptype, "Regular") )
	regular = true;
    else if ( !mMatchCI(sleeptype, "Extra") )
	partail = parnext;

    if ( regular && time<0.0 )
    {
	mParseErrStrm << "Regular sleep time must be a non-negative float"
		      << std::endl;
	return false;
    }

    mParTail( partail );
    setSleep( (float) time, regular );
    return true;
}


bool WaitCmd::act( const char* parstr )
{
    char* parnext;
    double time = strtod( parstr, &parnext );

    BufferString waittype; 
    const char* partail = getNextWord( parnext, waittype.buf() ); 
    bool regular = false;

    if ( mMatchCI(waittype, "Regular") )
	regular = true;
    else if ( !mMatchCI(waittype, "Extra") )
	partail = parnext;

    if ( regular && time<0.0 )
    {
	mParseErrStrm << "Regular wait time must be a non-negative float"
		      << std::endl;
	return false;
    }

    mParTail( partail );
    setWait( (float) time, regular );
    return true;
}


bool OnErrorCmd::act( const char* parstr )
{
    BufferString argword; 
    const char* partail = getNextWord( parstr, argword.buf() ); 

    if ( mMatchCI(argword,"Continue") )
    {
	mParseWarnStrm << "Continue-option no longer supported. "
		       << "Use \"Try\"-command ('~') instead" << std::endl;
	setOnError( CmdDriver::Recover );
    }
    else if ( mMatchCI(argword,"Recover") )
	setOnError( CmdDriver::Recover );
    else if ( mMatchCI(argword,"Stop") )
	setOnError( CmdDriver::Stop );
    else 
    {
	mParseErrStrm << "Argument not in {Stop, Recover}"
		      << std::endl;
	return false;
    }

    mParTail( partail );
    return true;
}


bool OnOffCheckCmd::act( const char* parstr )
{
    mParseWarnStrm << "This obsolete command is skipped" << std::endl;
    return true;
}


bool CaseCmd::act( const char* parstr )
{
    mParCase( parstr, partail, casesensitive, false );
    mParTail( partail );
    setCaseSensitive( casesensitive );
    return true;
}


bool GreyOutsCmd::act( const char* parstr )
{
    BufferString argword; 
    const char* partail = getNextWord( parstr, argword.buf() ); 

    if ( mMatchCI(argword,"Skip") )
	skipGreyOuts( true );
    else if ( mMatchCI(argword,"Count") )
	skipGreyOuts( false );
    else 
    {
	mParseErrStrm << "Argument not in {Count, Skip}"
		      << std::endl;
	return false;
    }

    mParTail( partail );
    return true;
}


bool IsMatchCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParDQuoted( "search key", parnext, parnexxt, searchkey, false, false );
    mParDQuoted( "search name", parnexxt, parnexxxt, searchnm, false, false );
    mParCase( parnexxxt, partail, casesensitive, true );
    mParTail( partail );

    if ( parnexxxt==partail )
	casesensitive = isCaseSensitive();

    const SearchKey key( searchkey, casesensitive ); 
    mParIdentPost( identname, (key.isMatching(searchnm) ? 1 : 0), parnext );
    wildcardMan().check( key, searchnm, false );
    return true;
}


bool LogModeCmd::act( const char* parstr )
{
    BufferString argword; 
    const char* partail = getNextWord( parstr, argword.buf() ); 

    if ( mMatchCI(argword,"Basic") )
	drv_.setLogMode( CmdDriver::LogBasic );
    else if ( mMatchCI(argword,"Normal") )
	drv_.setLogMode( CmdDriver::LogNormal );
    else if ( mMatchCI(argword,"All") )
	drv_.setLogMode( CmdDriver::LogAll );
    else
    {
	mParseErrStrm << "Argument not in {Basic, Normal, All}"
		      << std::endl;
	return false;
    }

    mParTail( partail );
    return true;
}


bool CommentCmd::act( const char* parstr )
{
    mParDQuoted( "text", parstr, partail, txtstr, false, true );
    mGetEscConvertedFMS( fms, txtstr, true );
    fms.setSepChar( '\n' );

    if ( parstr == partail )
    {
	mSkipBlanks( parstr );
	txtstr = parstr;
	mParseWarnStrm << "Missing double quotes (obsolete syntax)"
		       << std::endl;
    }
    else
    {
	mParTail( partail );
	txtstr = fms.unescapedStr();
    }

    mLogStrm << std::endl << txtstr << std::endl << std::endl;
    return true;
}


bool PauseCmd::act( const char* parstr )
{
    mParDQuoted( "text", parstr, partail, txtstr, false, true );
    mParTail( partail );

    mGetEscConvertedFMS( fms, txtstr, true );
    fms.setSepChar( '\n' );

    InteractSpec ispec;
    ispec.dlgtitle_ = "Execution of command script has been paused";
    ispec.infotext_ = fms.unescapedStr();
    ispec.cancelbuttext_ = "&Resume";

    interact( &ispec );

    return true;
}


bool GuideCmd::act( const char* parstr )
{
    mParDQuoted( "text", parstr, parnext, txtstr, false, false );

    BufferString winstatetag; 
    const char* parnexxt = getNextWord( parnext, winstatetag.buf() ); 

    WinStateType winstatetyp = NoState;

    if ( mMatchCI(winstatetag, "Existent") )
	winstatetyp = Existent;
    else if ( mMatchCI(winstatetag, "Inexistent") )
	winstatetyp = Inexistent;
    else if ( mMatchCI(winstatetag, "Accessible") )
	winstatetyp = Accessible;
    else if ( mMatchCI(winstatetag, "Inaccessible") )
	winstatetyp = Inaccessible;

    const bool userresume = winstatetyp == NoState;
    mParDQuoted( "window", parnexxt, partail, winstr, true, userresume );

    if ( !userresume )
    {
	BufferString dummywintitle = winstr;
	mParDisambiguator( "window name", dummywintitle, dummyselnr );
    }
    else
	partail = parnext;

    mParTail( partail );

    if ( !userresume && !verifyWinState(winstr, winstatetyp) )
    {
	mWinWarnStrm << "Guidance condition did not hold from start"
		     << std::endl;
	return true;
    }

    mGetEscConvertedFMS( fms, txtstr, true );
    fms.setSepChar( '\n' );

    InteractSpec ispec;
    ispec.launchpad_ = curWin();
    ispec.cursorshape_ = MouseCursor::Arrow;
    ispec.dlgtitle_ = "Guiding information";
    ispec.infotext_ = fms.unescapedStr();

    if ( !userresume )
    {
	if ( winstatetyp==Existent || winstatetyp==Accessible )
	{
	    ispec.resumetext_ = "This info bears on window \"";
	    ispec.resumetext_ += winstr; ispec.resumetext_ += "\"";
	}
    }
    else
	ispec.cancelbuttext_ = "&Done";

    interact( &ispec );
    return true;
}


bool AssignCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParExpr( false, identname, parnext, partail, valstr, false );
    mParTail( partail );
    identifierMan().set( identname, valstr );
    return true;
}


bool TryCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, true );
    const bool isdummy = identname == "_dummyvar";

    const bool ok = tryAction( identname, parnext );

    int tryoutres;
    identifierMan().getInteger( identname, tryoutres );

    if ( !ok && tryoutres==1 )
	return false;

    mTimeStrm << "TRY: " << parnext << " -->> " << (isdummy ? "" : identname)
	      << (isdummy ? "" : " = ") << tryoutres << std::endl;

    return true;
}


bool EndCmd::act( const char* partail )
{
    mParTail( partail );
    end();
    return true;
}


#define mNumCheck( name, valstr, num ) \
\
    double num; \
    if ( !StringProcessor(valstr).convertToDouble(&num) ) \
    { \
	mParseErrStrm << name << " needs numerical result" << std::endl; \
	return false; \
    }

bool IfCmd::act( const char* parstr )
{
    mParExpr( false, "", parstr, partail, valstr, false );
    mParTail( partail );
    mNumCheck( "Expression", valstr, res );

    if ( !res )
	jump();

    return true;
}



bool ElseIfCmd::act( const char* parstr )
{
    mParExpr( false, "", parstr, partail, valstr, false );
    mParTail( partail );
    mNumCheck( "Expression", valstr, res );

    if ( !res )
	jump();

    return true;
}


bool ElseCmd::act( const char* partail )
{
    mParTail( partail );
    return true;
}


bool FiCmd::act( const char* partail )
{
    mParTail( partail );
    return true;
}


bool DoCmd::act( const char* partail )
{
    mParTail( partail );
    return true;
}


bool OdUntilCmd::act( const char* parstr )
{
    mParExpr( false, "", parstr, partail, valstr, false );
    mParTail( partail );
    mNumCheck( "Expression", valstr, res );

    if ( !res )
	jump( 1 );

    return true;
}


bool DoWhileCmd::act( const char* parstr )
{
    mParExpr( false, "", parstr, partail, valstr, false );
    mParTail( partail );
    mNumCheck( "Expression", valstr, res );

    if ( !res )
	jump();

    return true;
}


bool OdCmd::act( const char* partail )
{
    mParTail( partail );
    return true;
}


bool ForCmd::act( const char* parstr )
{
    mSkipBlanks( parstr );
    BufferString altparstr( parstr );
    char* assignptr = (char *) StringProcessor(altparstr).findAssignment();
    if ( !assignptr )
    {
	mParseErrStrm << "Missing assignment symbol: '='" << std::endl;
	return false;
    }
    BufferString otherargs = assignptr+1;
    *assignptr = '\0';
    altparstr += altparstr.isEmpty() ? "_dummyvar " : " ";
    altparstr += otherargs;
    mParIdentInit( altparstr, parnext, identname, false );

    mParExpr( false, "", parnext, partail, fromdummy, true );
    BufferString fromexpr = exprInterpreter().parsedExpr();

    BufferString tostepword, toexpr, stepexpr, newexpr; 

    const char* parnexxt = getNextWord( partail, tostepword.buf() ); 
    if ( mMatchCI(tostepword, "To") )
    {
	mParExpr( false, "", parnexxt, parnexxxt, todummy, true );
	toexpr = exprInterpreter().parsedExpr();
	partail = parnexxxt;
    }

    stepexpr = 1;
    parnexxt = getNextWord( partail, tostepword.buf() );
    if ( mMatchCI(tostepword, "Step") )
    {
	mParExpr( false, "", parnexxt, parnexxxt, stepdummy, true );
	stepexpr = exprInterpreter().parsedExpr();
	partail = parnexxxt;
    }

    mParTail( partail );

    const bool initial = lastActionIdxMove() > 0;

    if ( initial )
    {
	mParExpr( false, identname, fromexpr, pardummy, fromstr, false );
	mNumCheck( "Start expression", fromstr, fromnum );
	identifierMan().set( identname, fromstr );
    }
    else
    {
	newexpr = identname; newexpr += " + ";
    }

    const char* identptr = initial ? "" : identname.buf();
    newexpr += "("; newexpr += stepexpr; newexpr += ")";
    mParExpr( false, identptr, newexpr, newdummy, newstr, false );
    mNumCheck( "Step expression", newstr, step );

    if ( !initial )
    {
	double oldval;
	identifierMan().getDouble( identname, oldval );
	step -= oldval;
	identifierMan().set( identname, newstr );
    }

    if ( !toexpr.isEmpty() )
    {
	BufferString condexpr = identname;
	condexpr += step>0 ? " <= (" : ( step<0 ? " >= (" : " != (" );
	condexpr += toexpr; condexpr += ")";
	mParExpr( false, "", condexpr, conddummy, valstr, false );
	mNumCheck( "Stop expression", valstr, res );
	if ( !res )
	    jump();
    }

    return true;
}


bool RofCmd::act( const char* partail )
{
    mParTail( partail );
    return true;
}


bool BreakCmd::act( const char* partail )
{
    mParTail( partail );
    jump();
    jump( 1 );
    return true;
}


bool ContinueCmd::act( const char* partail )
{
    mParTail( partail );
    jump();
    return true;
}

#define mParProcedure( typ, parstr, keyfms, parfms ) \
\
    FileMultiString keyfms, parfms; \
{ \
    const char missingchar = \
	       StringProcessor(parstr).preParseProcedure( keyfms, parfms ); \
\
    if ( missingchar ) \
    { \
	mParseErrStrm << "Procedure " << typ << " ended while searching " \
		      << "for '" << missingchar << "'-character" << std::endl; \
	return false; \
    } \
\
    BufferString procname = keyfms[0]; \
    removeTrailingBlanks( procname.buf() ); \
    const StringProcessor strproc( procname ); \
    BufferString ident; \
    const char* endptr = strproc.parseIdentifier( ident ); \
    if ( !endptr || *endptr ) \
    { \
	const char* assignptr = strproc.findAssignment( "=~" ); \
	if ( endptr && assignptr && !atoi(keyfms[1]) ) \
	{ \
	    mParseErrStrm << "Procedure " << typ << " contains '" \
			  << *assignptr << "'-operator instead of " \
			  << "'?'-operator" << std::endl; \
	    return false; \
	} \
\
	mParseErrStrm << (procname.isEmpty() ? "Missing procedure name" \
					     : "Invalid procedure name: ") \
		      << procname << std::endl; \
	return false; \
    } \
}

bool DefCmd::act( const char* parstr )
{
    mParProcedure( "header", parstr, keyfms, parfms );

    for ( int idx=0; idx<parfms.size(); idx++ )
    {
	BufferString param = parfms[idx];
	removeTrailingBlanks( param.buf() );
	if ( param == "_dummyvar" )
	    continue;

	BufferString ident;
	const char* endptr = StringProcessor(param).parseIdentifier( ident );
	if ( !endptr || *endptr )
	{
	    mParseErrStrm << (param.isEmpty() ? "Empty formal parameter" :
						"Invalid formal parameter: ")
			  << param << std::endl;
	    return false;
	}

	mUnscope( ident.buf(), unscoped );
	if ( ident.buf() != unscoped )
	{
	    mParseErrStrm << "A scope operator cannot precede a formal "
			  << "parameter: " << param << std::endl;
	    return false;
	}

	for ( int idy=0; idy<idx; idy++ )
	{
	    if ( param == parfms[idy] )
	    {
		mParseErrStrm << "Multiple occurrence of formal parameter: "
			      << param << std::endl;
		return false;
	    }
	}
    }

    parfms += curActionIdx();
    identifierMan().set( keyfms.buf(), parfms.buf() );

    jump();
    return true;
}


bool FedCmd::act( const char* partail )
{
    mParTail( partail );
    return true;
}


bool CallCmd::act( const char* parstr )
{
    mParProcedure( "call", parstr, keyfms, actualfms );

    const char* formalfmsptr = identifierMan().getValue( keyfms );

    if ( !formalfmsptr )
    {
	BufferString procname = keyfms[0];
	mParseErrStrm << "Procedure \"" << procname << "\" with "
		      << atoi(keyfms[1]) << " value and " << atoi(keyfms[2])
		      << " variable parameter(s) has not been defined"
		      << std::endl;
	return false;
    }

    FileMultiString evalfms, formalfms( formalfmsptr );

    for ( int idx=0; idx<actualfms.size(); idx++ )
    {
	BufferString actualpar = actualfms[idx];
	removeTrailingBlanks( actualpar.buf() );
	if ( idx>0 && idx<=atoi(keyfms[1]) )
	{
	    mParExpr( true, formalfms[idx], actualpar, dummy, valstr, false );
	    evalfms += valstr;
	}
	else
	{
	    mParIdentInit( actualpar, pardummy, identnm, true );
	    evalfms += identnm;
	    if ( actualpar != "_dummyvar" )
	    {
		if ( strcmp(formalfms[idx], "_dummyvar") ) 
		{
		    mTimeStrm << "LINK: " << formalfms[idx] << "' -> "
			      << actualpar << std::endl;
		}
		else
		{
		    mParseWarnStrm << "Procedure did not define return "
			<< "parameter to link: " << actualpar << std::endl;
		}
	    }
	}
    }

    identifierMan().raiseScopeLevel( true );
    identifierMan().set( "_returnvar", formalfms[0] );

    for ( int idx=0; idx<actualfms.size(); idx++ )
    {
	const bool islink = !idx || idx>atoi(keyfms[1]);
	identifierMan().set( formalfms[idx], evalfms[idx], islink );
    }

    if ( !insertProcedure(atoi(formalfms[formalfms.size()-1])) )
	pErrMsg( "Huh? Corrupted command action sequence!" );

    return true;
}


bool ReturnCmd::act( const char* parstr )
{
    mSkipBlanks( parstr );
    if ( parstr && *parstr )
    {
	const char* identnm = identifierMan().getValue( "_returnvar" );
	if ( !identnm || !strcmp(identnm, "_dummyvar") )
	{
	    mParseErrStrm << "Procedure did not define parameter to assign " 
			  << "a return value" << std::endl;
	    return false;
	}

	mParExpr( false, identnm, parstr, partail, valstr, false );
	mParTail( partail );
	identifierMan().set( identnm, valstr ); 
    }

    identifierMan().raiseScopeLevel( false );
    jump();
    jump( 1 );
    return true;
}


}; // namespace CmdDrive
