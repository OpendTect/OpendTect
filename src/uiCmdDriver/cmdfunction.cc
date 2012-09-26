/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		April 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "cmdfunction.h"

#include "cmddriverbasics.h"
#include "cmddriver.h"
#include "searchkey.h"

#include "math.h"
#include "statrand.h"
#include "typeset.h"


namespace CmdDrive
{

mImplFactory1Param( Function, const CmdDriver&, Function::factory );


BufferString Function::factoryKey( const char* name )
{
    mUnscope( name, unscoped );
    BufferString fackey = unscoped;
    StringProcessor(fackey).capitalize();
    return fackey;
}


BufferString Function::createFactoryKey( const char* keyword )
{
    const BufferString fackey = factoryKey( keyword );

    if ( factory().hasName(fackey) )
    {
	BufferString errmsg( "Redefining function \"" );
	errmsg += keyword; errmsg += "\"";
	pFreeFnErrMsg( errmsg, "CmdDrive::Function" );
    }

    return fackey;
}


void Function::initStandardFunctions()
{
    static bool done = false;
    if ( done ) return;
    done = true;

    AbsFunc::initClass();
    AsinFunc::initClass();
    AcosFunc::initClass();
    AtanFunc::initClass();
    CeilFunc::initClass();
    CosFunc::initClass();
    ExpFunc::initClass();
    FloorFunc::initClass();
    LnFunc::initClass();
    LogFunc::initClass();
    RoundFunc::initClass();
    SgnFunc::initClass();
    SinFunc::initClass();
    SqrtFunc::initClass();
    TanFunc::initClass();
    TruncFunc::initClass();

    RandFunc::initClass();
    RandGFunc::initClass();
    Atan2Func::initClass();

    AvgFunc::initClass();
    MaxFunc::initClass();
    MedFunc::initClass();
    MinFunc::initClass();
    SumFunc::initClass();
    VarFunc::initClass();

    IsAlNumFunc::initClass();
    IsAlphaFunc::initClass();
    IsDigitFunc::initClass();
    IsLowerFunc::initClass();
    IsSpaceFunc::initClass();
    IsUpperFunc::initClass();

    ToLowerFunc::initClass();
    ToUpperFunc::initClass();

    IsNumberFunc::initClass();
    IsIntegerFunc::initClass();

    StrCatFunc::initClass();
    StrLenFunc::initClass();
    StrSelFunc::initClass();

    SepStrCatFunc::initClass();
    SepStrLenFunc::initClass();
    SepStrSelFunc::initClass();

    WildcardFunc::initClass();
    WildcardStrFunc::initClass();

    CurWindowFunc::initClass();
}


bool Function::openQDlg() const
{ return drv_.openQDlg(); }

const uiMainWin* Function::curWin() const
{ return drv_.curWin(); }

const uiMainWin* Function::applWin() const
{ return drv_.applWin(); }

const WildcardManager& Function::wildcardMan() const
{ return drv_.wildcardMan(); }


#define mCheckArgs( nrargs, cmpoperator, musthavetxt, args, res ) \
\
    if ( args.size() cmpoperator (nrargs) ) \
    { \
	res = "~"; res += name(); res += "-function must have "; \
	res += musthavetxt; res += nrargs; \
	res += (nrargs==1 ? " argument" : " arguments");  \
	return false; \
    }

#define mCheckNrArgs( nrargs, args, res ) \
    mCheckArgs( nrargs, !=, "", args, res )

#define mCheckMinArgs( minargs, args, res ) \
    mCheckArgs( minargs, <, "at least ", args, res )

#define mCheckMaxArgs( maxargs, args, res ) \
    mCheckArgs( maxargs, >, "at most ", args, res )


#define mGetNumArg( idx, num, args, res ) \
\
    double num; \
    if ( idx<args.size() && \
	 !StringProcessor(args.get(idx)).convertToDouble(&num) ) \
    { \
	res = "~"; res += name(); res += "-function cannot convert argument "; \
	res += idx+1; res += " to numerical"; \
	return false; \
    }

#define mGetIntArg( idx, num, args, res ) \
\
    int num; \
    if ( idx<args.size() && \
	 !StringProcessor(args.get(idx)).convertToInt(&num) ) \
    { \
	res = "~"; res += name(); res += "-function cannot convert argument "; \
	res += idx+1; res += " to integer"; \
	return false; \
    }

#define mDefMathFunc( clss, errcondition, errmsgtail, resaction ) \
\
bool clss##Func::eval( const BufferStringSet& args, BufferString& res ) const \
{ \
    mCheckNrArgs( 1, args, res ); \
    mGetNumArg( 0, num, args, res ); \
    if ( mIsUdf(num) ) \
    { \
	res = num; \
       	return true; \
    } \
    if ( errcondition ) \
    { \
	res = name(); res += "-function requires "; res += errmsgtail; \
	return false; \
    } \
    resaction; \
    return true; \
}

mDefMathFunc( Abs,   false, "", res=(num<0 ? -num : num) );
mDefMathFunc( Asin,  fabs(num)>1, "argument in range [-1,1]", res=asin(num) );
mDefMathFunc( Acos,  fabs(num)>1, "argument in range [-1,1]", res=acos(num) );
mDefMathFunc( Atan,  false, "", res=atan(num) );
mDefMathFunc( Ceil,  false, "", res=ceil(num) );
mDefMathFunc( Cos,   false, "", res=cos(num) );
mDefMathFunc( Exp,   false, "", res=exp(num) );
mDefMathFunc( Floor, false, "", res=floor(num) );
mDefMathFunc( Ln,    num<=0, "positive argument", res=log(num) );
mDefMathFunc( Log,   num<=0, "positive argument", res=log10(num) );
mDefMathFunc( Round, false, "", res=(num<0 ? ceil(num-0.5) : floor(num+0.5)) );
mDefMathFunc( Sgn,   false, "", res=(!num ? 0 : (num<0 ? -1 : 1)) );
mDefMathFunc( Sin,   false, "", res=sin(num) );
mDefMathFunc( Sqrt,  num<0, "non-negative argument", res=sqrt(num) );
mDefMathFunc( Tan,   false, "", res=tan(num) );
mDefMathFunc( Trunc, false, "", res=(num<0 ? ceil(num) : floor(num)) );


#define mDefRandFunc( clss, funcall ) \
\
bool clss##Func::eval( const BufferStringSet& args, BufferString& res ) const \
{ \
    mCheckMaxArgs( 1, args, res ); \
    mGetNumArg( 0, num, args, res ); \
    if ( !args.size() ) \
	num = 1.0; \
\
    res = mIsUdf(num) ? num : num*Stats::RandGen::funcall; \
    return true; \
}

mDefRandFunc( Rand, get() );
mDefRandFunc( RandG, getNormal(0,1) );


bool Atan2Func::eval( const BufferStringSet& args, BufferString& res ) const
{
    mCheckNrArgs( 2, args, res );
    mGetNumArg( 0, num0, args, res ); 
    mGetNumArg( 1, num1, args, res ); 

    if ( !num0 && !num1 )
    {
	res = name(); res += "-function does not allow both arguments zero";
	return false;
    }

    res = mIsUdf(num0) ? num0 : ( mIsUdf(num1) ? num1 : atan2(num0,num1) );
    return true;
}


#define mDefStatFunc( clss, loopaction, resaction ) \
\
bool clss##Func::eval( const BufferStringSet& args, BufferString& res ) const \
{ \
    mCheckMinArgs( 1, args, res ); \
    const int sz = args.size(); \
    TypeSet<double> ts; \
    double mUnusedVar a1 = 0.0; \
    double mUnusedVar a2 = 0.0; \
    for ( int idx=0; idx<sz; idx++ ) \
    { \
	mGetNumArg( idx, num, args, res ); \
	if ( mIsUdf(num) ) \
	{ \
	    res = num; \
	    return true; \
	} \
	loopaction; \
    } \
    resaction; \
    return true; \
}

mDefStatFunc( Avg, a1+=num, res=a1/sz );
mDefStatFunc( Max, a1=(idx ? mMAX(a1,num) : a1), res=a1 );
mDefStatFunc( Min, a1=(idx ? mMIN(a1,num) : a1), res=a1 );
mDefStatFunc( Sum, a1+=num, res=a1 );
mDefStatFunc( Var, a1+=num; a2+=num*num, res=(a2-a1*a1/sz)/sz );
mDefStatFunc( Med, ts+=num,
	      sort(ts); res=(sz%2 ? ts[sz/2] : (ts[sz/2-1]+ts[sz/2])/2) );


#define mDefCharFunc( clss, funcname ) \
\
bool clss##Func::eval( const BufferStringSet& args, BufferString& res ) const \
{ \
    mCheckNrArgs( 1, args, res ); \
    res = args.get(0).isEmpty() ? 0 : 1; \
    for ( int idx=0; idx<args.get(0).size(); idx++ ) \
    { \
	if ( !funcname(args.get(0)[idx]) ) \
	    res = 0; \
    } \
    return true; \
}

mDefCharFunc( IsAlNum, isalnum );
mDefCharFunc( IsAlpha, isalpha );
mDefCharFunc( IsDigit, isdigit );
mDefCharFunc( IsLower, islower );
mDefCharFunc( IsSpace, isspace );
mDefCharFunc( IsUpper, isupper );


#define mDefToFunc( clss, funcname ) \
\
bool clss##Func::eval( const BufferStringSet& args, BufferString& res ) const \
{ \
    mCheckNrArgs( 1, args, res ); \
    res.setEmpty(); \
    for ( int idx=0; idx<args.get(0).size(); idx++ ) \
	mAddCharToBufStr( res, funcname(args.get(0)[idx]) ); \
\
    return true; \
}

mDefToFunc( ToLower, tolower );
mDefToFunc( ToUpper, toupper );


#define mDefIsFunc( clss, funcname ) \
\
bool clss##Func::eval( const BufferStringSet& args, BufferString& res ) const \
{ \
    mCheckNrArgs( 1, args, res ); \
    res = (int) StringProcessor(args.get(0)).funcname(); \
    return true; \
}

mDefIsFunc( IsNumber, convertToDouble );
mDefIsFunc( IsInteger, convertToInt );


bool StrCatFunc::eval( const BufferStringSet& args, BufferString& res ) const
{
    mCheckMinArgs( 1, args, res );

    res.setEmpty();
    for ( int idx=0; idx<args.size(); idx++ )
	StringProcessor(res).appendCharElements( args.get(idx) );

    return true;
}


bool StrLenFunc::eval( const BufferStringSet& args, BufferString& res ) const
{
    mCheckNrArgs( 1, args, res );
    res = StringProcessor( args.get(0) ).nrCharElements();
    return true;
}


#define  mCheckPosRange( prefix, pos, len, args, res ) \
\
    if ( !pos || abs(pos)>len ) \
    { \
	res += prefix; res += " position: "; res += pos; \
	res += pos<0 ? " not in range [-1,-" : " not in range [1," ; \
	res += len; res += "]"; \
	return false; \
    }


bool StrSelFunc::eval( const BufferStringSet& args, BufferString& res ) const
{
    mCheckMinArgs( 2, args, res );
    mCheckMaxArgs( 3, args, res );

    const int len = StringProcessor(args.get(0)).nrCharElements();
    if ( !len )
    {
	res = "Nothing to select in empty string";
	return false;
    }

    mGetIntArg( 1, startpos, args, res );
    mGetIntArg( 2, stoppos, args, res );

    if ( args.size() != 3 )
	stoppos = startpos;
    
    const char* prefix = args.size()==3 ? "First character" : "Character";
    mCheckPosRange( prefix, startpos, len, args , res );
    mCheckPosRange( "Last character", stoppos, len, args , res );

    int startidx = startpos>0 ? startpos-1 : len+startpos;
    int stopidx = stoppos>0 ? stoppos-1 : len+stoppos;

    res.setEmpty();
    const int dir = startidx>stopidx ? -1 : 1;
    for ( int idx=startidx; dir*idx<=dir*stopidx; idx+=dir )
	res += StringProcessor(args.get(0)).getCharElement( idx );

    return true;
}


bool SepStrCatFunc::eval( const BufferStringSet& args, BufferString& res ) const
{
    mCheckMinArgs( 1, args, res );

    res = args.get(0);
    for ( int idx=1; idx<args.size(); idx++ )
    {
	StringProcessor(res).appendCharElements( "`" );
	StringProcessor(res).appendCharElements( args.get(idx) );
    }

    return true;
}


bool SepStrLenFunc::eval( const BufferStringSet& args, BufferString& res ) const
{
    mCheckNrArgs( 1, args, res );

    mGetEscConvertedFMS( fms, args.get(0), false );
    res = mSepStrSize( fms );
    return true;
}


bool SepStrSelFunc::eval( const BufferStringSet& args, BufferString& res ) const
{
    mCheckMinArgs( 2, args, res );
    mCheckMaxArgs( 3, args, res );

    mGetEscConvertedFMS( fms, args.get(0), false );
    const int len = mSepStrSize( fms );

    mGetIntArg( 1, startpos, args, res );
    mGetIntArg( 2, stoppos, args, res );

    if ( args.size() != 3 )
	stoppos = startpos;
    
    const char* prefix = args.size()==3 ? "First substring" : "Substring";
    mCheckPosRange( prefix, startpos, len, args , res );
    mCheckPosRange( "Last substring", stoppos, len, args , res );

    int startidx = startpos>0 ? startpos-1 : len+startpos;
    int stopidx = stoppos>0 ? stoppos-1 : len+stoppos;

    res = fms[startidx];
    const int dir = startidx>stopidx ? -1 : 1;
    for ( int idx=startidx+dir; dir*idx<=dir*stopidx; idx+=dir )
    {
	StringProcessor(res).appendCharElements( "`" );
	StringProcessor(res).appendCharElements( fms[idx] );
    }

    return true;
}


#define mDefWildcardFunc( clss, funcname ) \
bool clss##Func::eval( const BufferStringSet& args, BufferString& res ) const \
{ \
    mCheckMaxArgs( 1, args, res ); \
    mGetIntArg( 0, wildcardnr, args, res ); \
\
    if ( !args.size() ) \
	wildcardnr = 1; \
\
    if ( !wildcardnr ) \
    { \
	res = "Wildcard number must be non-zero integer"; \
	return false; \
    } \
\
    const int nrwildcards = wildcardMan().nrWildcards(); \
    const int idx = wildcardnr>0 ? wildcardnr-1 : nrwildcards+wildcardnr; \
\
    if ( idx<0 || idx>=wildcardMan().nrWildcards() ) \
    { \
	if ( wildcardMan().nrWildcards() ) \
	{ \
	    res = "Last successful wildcard action had only "; \
	    res += nrwildcards; res += " wildcard"; \
	    res += (nrwildcards>1) ? "s" : ""; \
	} \
	else \
	    res = "No action did assign wildcards so far"; \
\
	return false; \
    } \
\
    res = *wildcardMan().funcname( idx ); \
    return true; \
}

mDefWildcardFunc( Wildcard, wildcard );
mDefWildcardFunc( WildcardStr, wildcardStr );


bool CurWindowFunc::eval( const BufferStringSet& args, BufferString& res ) const
{
    mCheckNrArgs( 0, args, res );
    res = windowTitle( applWin(), (openQDlg() ? 0 : curWin()) );
    StringProcessor(res).addCmdFileEscapes( StringProcessor::sAllEscSymbols() );
    return true;
}


}; // namespace CmdDrive
