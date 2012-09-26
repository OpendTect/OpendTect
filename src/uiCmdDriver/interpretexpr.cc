/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		March 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "interpretexpr.h"

#include "cmddriver.h"
#include "cmdfunction.h"
#include "identifierman.h"
#include "math.h"


namespace CmdDrive
{


ExprInterpreter::ExprInterpreter( const CmdDriver& cmddrv )
    : drv_( cmddrv )
{}


bool ExprInterpreter::isParseError() const
{ return *errmsg_.buf() == '~'; }


const char* ExprInterpreter::errMsg() const
{
    const int offset = *errmsg_.buf() && mIsSymbol(*errmsg_.buf()) ? 1 : 0;
    return errmsg_.buf() + offset;
}


#define mErrRet( endptr, errmsg ) \
{ \
    setBreakPrefix( endptr, breakprefix_ ); \
    errmsg_ = errmsg; \
    return 0; \
}

const char* ExprInterpreter::process( const char* exprstr, BufferString& val,
       				      bool isargument )
{
    exprstr_ = exprstr;
    mSkipBlanks( exprstr_ );
    trivialresult_ = true;
    parsedexpr_.setEmpty();
    breakprefix_.setEmpty();
    errmsg_.setEmpty();

    const char* endptr = interpretCompositeExpr( exprstr_, val );

    if ( !endptr )
	return 0;

    if ( *endptr && isargument )
	mErrRet( endptr, "~Expect valid operator, comma or right parenthesis" );

    if ( *endptr && !isargument && !isspace(*endptr) )
	mErrRet( endptr, "~Expect valid operator or white space" );

    setBreakPrefix( endptr, parsedexpr_ );
    return endptr;
}


const char* ExprInterpreter::interpretSingleExpr( const char* parstr,
						  BufferString& val )
{
    mSkipBlanks( parstr );
    val.setEmpty();

    const char* parnext = StringProcessor(parstr).parseDQuoted( val );
    if ( parnext )
	return parnext;

    if ( *parstr == '"' )
	mErrRet( parstr+1, "~Missing right double-quote" );

    char* doubleparnext;
    const double doublenum = strtod( parstr, &doubleparnext );
    char* intparnext;
    const int intnum = strtol( parstr, &intparnext, 0 );

    if ( parstr!=doubleparnext || parstr!=intparnext )
    {
	if ( intparnext < doubleparnext )
	{
	    val += doublenum;
	    return doubleparnext;
	}

	val += intnum;
	return intparnext;
    }

    BufferString name;
    parnext = StringProcessor(parstr).parseIdentifier( name );
    if ( parnext )
    {
	if ( *parnext == '(' )
	{
	    BufferStringSet args;
	    BufferString breakprefix1, errmsg1, res;

	    const BufferString fackey = Function::factoryKey( name );
	    PtrMan<Function> func = Function::factory().create( fackey, drv_ );
	    if ( !func )
		mErrRet( parnext+1, "~Unknown built-in function" );

	    parnext++;
	    mSkipBlanks( parnext );

	    if ( *parnext != ')' )
	    {
		while ( true )
		{
		    const char* endptr = interpretCompositeExpr( parnext, val );
		    if ( !endptr )
			return 0;

		    if ( errmsg1.isEmpty() )
		    {
			args.add( val );
			breakprefix1 = breakprefix_;
			errmsg1 = errmsg_;
		    }
		    breakprefix_.setEmpty();
		    errmsg_.setEmpty();

		    parnext = endptr;
		    mSkipBlanks( parnext );

		    if ( *parnext == ',' )
		    {
			parnext++; 
			continue;
		    }
		    if ( *parnext == ')' )
			break;

		    mErrRet( endptr,
			"~Expect valid operator, comma or right parenthesis" );
		}
	    }

	    if ( !errmsg1.isEmpty() )
	    {
		breakprefix_ = breakprefix1;
		errmsg_ = errmsg1;
	    }
	    else if ( !func->eval(args, res) )
	    {
		setBreakPrefix( parnext+1, breakprefix_ );
		errmsg_ = res;
	    }
	    else
		val = res;

	    trivialresult_ = false;
	    return parnext+1;
	}
	else if ( *parnext == '[' )
	{
	    errmsg_ = "~If an array variable a[i] was intended, ";
	    errmsg_ += "use index substitution a_$i$ instead";
	    mErrRet( parnext+1, errmsg_ );
	}
	else
	{
	    const char* valstr = drv_.identifierMan().getValue( name );
	    if ( valstr )
	    {
		val = valstr;
		trivialresult_ = false;
	    }
	    else
	    {
		setBreakPrefix( parnext, breakprefix_ );
		errmsg_ = "~Unknown identifier: ";
		errmsg_ += drv_.identifierMan().lastLinkedIdentStr();
	    }
	    return parnext;
	}
    }

    if ( *parstr == '(' )
    {
	parnext = interpretCompositeExpr( parstr+1, val );
	if ( !parnext )
	    return 0;

	mSkipBlanks( parnext );

	if ( *parnext != ')' )
	    mErrRet( parnext, "~Expect valid operator or right parenthesis" )
	else
	    return parnext+1;
    }

    if ( *parstr == '|' )
	mErrRet( parnext+1, "~If |x| was intended, use abs(x) instead" );

    if ( *parstr=='!' || *parstr=='+' || *parstr=='-' )
    { 
	BufferString valstr;
	parnext = interpretSingleExpr( parstr+1, valstr );
	if ( !parnext )
	    return 0;

	double num;
	if ( StringProcessor(valstr).convertToDouble(&num) )
	{
	    if ( mIsUdf(num) || *parstr=='+' )
		val = num;
	    else
		val = *parstr=='!' ? !num : -num;

	    trivialresult_ = false;
	}
	else
	{
	    setBreakPrefix( parstr+1, breakprefix_ );
	    errmsg_= "~Unary operator cannot convert its argument to numerical";
	}
	return parnext;
    }

    if ( parstr == exprstr_ )
	mErrRet( parstr, "~Invalid expression" );

    mErrRet( parstr, "~Invalid sub-expression" );
}


enum InfixOperator {
    	IfFalse=0, IfTrue=10, Or=20, And=30, Equal=40, NotEqual,
	Less=50, LessEqual, Greater, GreaterEqual, Add=60, Subtract,
	Multiply=70, Divide, IntDiv, Mod, Power=80 };

#define mMatchOperator( opstr, parstr, optag ) \
\
    if ( matchString(opstr, parstr) ) \
    { \
	parstr += strlen( opstr ); \
	ops += optag; \
	opptrs += parstr; \
    }


#define mRemoveArg( argidx ) \
{ \
    vals.remove( opidx+argidx ); \
    breakprefixes.remove( opidx+argidx ); \
    errmsgs.remove( opidx+argidx ); \
    ops.remove( opidx ); \
    opptrs.remove( opidx ); \
}

#define mIfErrArgCont( argidx ) \
\
    if ( !errmsgs.get(opidx+argidx).isEmpty() ) \
    { \
	mRemoveArg( argidx ? 0 : 1 ); \
	continue; \
    }

#define mErrCont( errmsg ) \
{ \
    vals.get(opidx).setEmpty(); \
    setBreakPrefix( opptrs[opidx], breakprefixes.get(opidx) ); \
    errmsgs.get(opidx) = errmsg; \
    mRemoveArg( 1 ); \
    continue; \
}

#define mNumArgErrCont( argidx ) \
{ \
    BufferString errmsg = "~Infix operator cannot convert argument "; \
    errmsg += argidx+1; errmsg += " to numerical"; \
    mErrCont( errmsg ); \
}

const char* ExprInterpreter::interpretCompositeExpr( const char* parstr,
						     BufferString& val )
{
    BufferStringSet vals;
    BufferStringSet breakprefixes;
    BufferStringSet errmsgs;

    TypeSet<InfixOperator> ops;
    ObjectSet<const char> opptrs;

    while ( true )
    {
	BufferString valstr;
	const char* parnext = interpretSingleExpr( parstr, valstr );
	if ( !parnext )
	    return 0;

	vals.add( valstr );
	breakprefixes.add( breakprefix_ );
	errmsgs.add( errmsg_ );

	breakprefix_.setEmpty();
	errmsg_.setEmpty();

	parstr = parnext;
	mSkipBlanks( parnext );

	     mMatchOperator( "||", parnext, Or )
	else mMatchOperator( "&&", parnext, And )
	else mMatchOperator( "==", parnext, Equal )
	else mMatchOperator( "!=", parnext, NotEqual )
	else mMatchOperator( "<=", parnext, LessEqual )
	else mMatchOperator( ">=", parnext, GreaterEqual )
	else mMatchOperator( "?",  parnext, IfTrue )
	else mMatchOperator( ":",  parnext, IfFalse )
	else mMatchOperator( "<",  parnext, Less )
	else mMatchOperator( ">",  parnext, Greater )
	else mMatchOperator( "+",  parnext, Add )
	else mMatchOperator( "-",  parnext, Subtract )
	else mMatchOperator( "*",  parnext, Multiply )
	else mMatchOperator( "/",  parnext, Divide )
	else mMatchOperator( "|",  parnext, IntDiv )
	else mMatchOperator( "%",  parnext, Mod )
	else mMatchOperator( "^",  parnext, Power )
	else break;

	trivialresult_ = false;
	parstr = parnext;
    }

    while ( !ops.isEmpty() )
    {
	int opidx = 0;
	for ( int idx=1; idx<ops.size(); idx++ )
	{
	    const int prioritydif = ops[idx]/10 - ops[opidx]/10;
	    const bool rightassoc = ops[idx]==IfTrue || ops[idx]==Power;

	    if ( prioritydif>0 || (!prioritydif && rightassoc) )
		opidx = idx;
	}

	double num0, num1;
	double res = mUdf(double);

	const bool isnum0 =
		StringProcessor(vals.get(opidx)).convertToDouble( &num0 );
	const bool isnum1 =
		StringProcessor(vals.get(opidx+1)).convertToDouble( &num1 );

	if ( ops[opidx] == IfFalse )
	{
	    mErrRet( opptrs[opidx],
		     "~Conditional operator: colon without question mark" );
	}

	if ( ops[opidx] == IfTrue )
	{
	    if ( opidx+1>=ops.size() || ops[opidx+1]!=IfFalse )
	    {
		mErrRet( opptrs[opidx],
			 "~Conditional operator: question mark without colon" );
	    }

	    if ( isnum0 && !mIsUdf(num0) )
	    {
		mRemoveArg( num0 ? 2 : 1 );
		mRemoveArg( 0 );
	    }
	    else
	    {
		mRemoveArg( 2 );
		mIfErrArgCont( 0 );

		if ( !isnum0 )
		    mNumArgErrCont( 0 )
		else
		    mRemoveArg( 1 );
	    }
	    continue;
	}

	mIfErrArgCont( 0 );

	if ( ops[opidx]==Or && isnum0 && !mIsUdf(num0) && num0 )
	{
	    mRemoveArg( 1 );
	    vals.get(opidx) = 1;
	    continue;
	}

	if ( ops[opidx]==And && isnum0 && !num0 )
	{
	    mRemoveArg( 1 );
	    continue;
	}

	mIfErrArgCont( 1 );

	if ( ops[opidx]==NotEqual || ops[opidx]==Equal )
	{
	    res = num0 == num1;

	    if ( !isnum0 || !isnum1 )
	    {
		StringProcessor(vals.get(opidx)  ).removeCmdFileEscapes();
		StringProcessor(vals.get(opidx+1)).removeCmdFileEscapes();
		res = vals.get(opidx) == vals.get(opidx+1);
	    }
	    if ( ops[opidx] == NotEqual )
		res = !res;
	}
	else if ( !isnum0 )
	    mNumArgErrCont( 0 )
	else if ( !isnum1 )
	    mNumArgErrCont( 1 );

	if ( ops[opidx] == Or )
	{
	    if ( mIsUdf(num0) )
		res = !num1 || mIsUdf(num1) ? num0 : 1;
	    else
		res = mIsUdf(num1) ? num1 : !!num1;
	}

	if ( ops[opidx] == And )
	{
	    if ( mIsUdf(num0) )
		res = num1 ? num0 : 0;
	    else 
		res = mIsUdf(num1) ? num1 : !!num1;
	}

	if ( !mIsUdf(num0) && !mIsUdf(num1) )
	{
	    if ( ops[opidx] == Less )
		res = num0 < num1;
	    if ( ops[opidx] == LessEqual )
		res = num0 <= num1;
	    if ( ops[opidx] == Greater )
		res = num0 > num1;
	    if ( ops[opidx] == GreaterEqual )
		res = num0 >= num1;
	    if ( ops[opidx] == Add )
		res = num0 + num1;
	    if ( ops[opidx] == Subtract )
		res = num0 - num1;
	    if ( ops[opidx] == Multiply )
		res = num0 * num1;

	    if ( ops[opidx]==Divide || ops[opidx]==IntDiv || ops[opidx]==Mod )
	    {
		if ( !num1 )
		    mErrCont( "Division by zero" );

		res = num0 / num1;

		if ( ops[opidx] != Divide )
		{
		    double intres = floor( res );
		    res = ops[opidx]==IntDiv ? intres : num0-intres*num1;
		}
	    }

	    if ( ops[opidx] == Power )
	    {
		if ( num0<0 && num1!=floor(num1) )
		    mErrCont( "Negative base requires integer exponent" )
		if ( !num0 && num1<0 )
		    mErrCont( "Negative exponent requires non-zero base" );

		res = pow( num0, num1 );
	    }
	}

	vals.get( opidx ) = res;
	mRemoveArg( 1 );
    }

    val = vals.get( 0 );
    breakprefix_ = breakprefixes.get( 0 );
    errmsg_ = errmsgs.get( 0 );
    return parstr;
}


void ExprInterpreter::setBreakPrefix( const char* endptr,
				      BufferString& breakprefix ) const
{
    breakprefix.setEmpty();
    const int sz = endptr - exprstr_; 
    if ( sz > 0 )
    {
	breakprefix.setBufSize( sz+1 );
	strncpy( breakprefix.buf(), exprstr_, sz );
	*( breakprefix.buf()+sz ) = '\0';
	removeTrailingBlanks( breakprefix.buf() );
    }
}


}; // namespace CmdDrive
