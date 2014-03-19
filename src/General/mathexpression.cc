/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Mar 2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "mathexpression.h"
#include "ctype.h"
#include "ptrman.h"
#include "math2.h"
#include "statrand.h"
#include "statruncalc.h"
#include "undefval.h"
#include "bufstring.h"

#ifndef M_PI
# define M_PI		3.14159265358979323846
#endif


const ObjectSet<const MathExpressionOperatorDescGroup>&
			MathExpressionOperatorDescGroup::supported()
{
    mDefineStaticLocalObject(
	PtrMan<ManagedObjectSet<const MathExpressionOperatorDescGroup> >,
	ret, = 0 );

    if ( ret ) return *ret;
    ret = new ManagedObjectSet<const MathExpressionOperatorDescGroup>;

    MathExpressionOperatorDescGroup* grp = new MathExpressionOperatorDescGroup;
    grp->name_ = "Basic";

#   define mAddDesc(s,d,i,n) \
    grp->opers_ += new MathExpressionOperatorDesc(s,d,i,n)
    mAddDesc("+","Add",true,2);
    mAddDesc("-","Subtract",true,2);
    mAddDesc("*","Multiply",true,2);
    mAddDesc("/","Divide",true,2);
    mAddDesc("^","Power",true,2);
    *ret += grp;

    grp = new MathExpressionOperatorDescGroup;
    grp->name_ = "Logical";
    mAddDesc("<","Less than",true,2);
    mAddDesc(">","Greater than",true,2);
    mAddDesc("<=","Less than or equal",true,2);
    mAddDesc(">=","Greater than or equal",true,2);
    mAddDesc("==","Equal",true,2);
    mAddDesc("!=","Not equal",true,2);
    mAddDesc("? :","Conditional value",true,3);
    mAddDesc("&&","AND",true,2);
    mAddDesc("||","OR",true,2);
    *ret += grp;

    grp = new MathExpressionOperatorDescGroup;
    grp->name_ = "MathFunctions";
    mAddDesc("sqrt","Square root",false,1);
    mAddDesc("exp","E-power",false,1);
    mAddDesc("ln","Natural logarithm",false,1);
    mAddDesc("log","10-Logarithm",false,1);
    mAddDesc("sin","Sinus",false,1);
    mAddDesc("cos","Cosinus",false,1);
    mAddDesc("tan","Tangent",false,1);
    mAddDesc("asin","ArcSinus",false,1);
    mAddDesc("acos","ArcCosinus",false,1);
    mAddDesc("atan","ArcTangent",false,1);
    mAddDesc("abs","Absolute value",false,1);
    mAddDesc("idiv","Integer division",false,2);
    mAddDesc("mod","Modulo, rest after integer division",false,2);
    *ret += grp;

    grp = new MathExpressionOperatorDescGroup;
    grp->name_ = "Statistical";
    mAddDesc("rand","Random number",false,0);
    mAddDesc("randg","Gaussian random number",false,0);
    mAddDesc("min","Minimum",false,2);
    mAddDesc("max","Maximum",false,2);
    mAddDesc("sum","Sum",false,3);
    mAddDesc("avg","Average",false,3);
    mAddDesc("med","Median",false,3);
    mAddDesc("var","Variance",false,3);
    *ret += grp;

    grp = new MathExpressionOperatorDescGroup;
    grp->name_ = "Various";
    mAddDesc("pi","PI",true,0);
    mAddDesc("undef","Undefined",true,0);
    *ret += grp;

    return *ret;
}



class MathExpressionVariable : public MathExpression
{
public:
				MathExpressionVariable( const char* str )
				    : MathExpression( 0 )
				    , str_(str) { addIfOK(str_.buf()); }

    const char*			fullVariableExpression( int ) const
							{ return str_.buf(); }

    int	nrVariables() const	{ return 1; }

    double			getValue() const	{ return val_; }

    void			setVariableValue( int, double nv )
							{ val_ = nv; }

    MathExpression*		clone() const
				{
				    MathExpression* res =
					new MathExpressionVariable(str_.buf());
				    copyInput( res );
				    return res;
				}

protected:

    double			val_;
    BufferString		str_;

};


class MathExpressionConstant : public MathExpression
{
public:

MathExpressionConstant( double val )
    : val_ ( val )
    , MathExpression( 0 )
{
}

double getValue() const
{
    return val_;
}

MathExpression* clone() const
{
    MathExpression* res = new MathExpressionConstant(val_);
    copyInput( res );
    return res;
}

void dumpSpecifics( BufferString& str, int nrtabs ) const
{
    str.addTab( nrtabs ).add( "Const value: " ).add( val_ ).addNewLine();
}

protected:

    double			val_;

};


#define mMathExpressionClass( clss, nr ) \
     \
class MathExpression##clss : public MathExpression \
{ \
public: \
MathExpression##clss() : MathExpression( nr ) \
{ \
} \
 \
double getValue() const; \
 \
MathExpression* clone() const \
{ \
    MathExpression* res = new MathExpression##clss(); \
    copyInput( res ); \
    return res; \
} \
 \
};


mMathExpressionClass( Plus, 2 )
double MathExpressionPlus::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);
    return val0+val1;
}


mMathExpressionClass( Minus, 2 )
double MathExpressionMinus::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return val0-val1;
}


mMathExpressionClass( Multiply, 2 )
double MathExpressionMultiply::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return val0*val1;
}


mMathExpressionClass( Divide, 2 )
double MathExpressionDivide::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    if ( mIsZero(val1,mDefEps) )
	return mIsZero(val0,mDefEps) ? 1 : mUdf(double);

    return val0/val1;
}


mMathExpressionClass( IntDivide, 2 )
double MathExpressionIntDivide::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    const od_int64 i0 = mRounded(od_int64,val0);
    const od_int64 i1 = mRounded(od_int64,val1);
    if ( i1 == 0 )
	return mUdf(double);

    return (double)( i0 / i1 );
}


mMathExpressionClass( IntDivRest, 2 )
double MathExpressionIntDivRest::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    const od_int64 i0 = mRounded(od_int64,val0);
    const od_int64 i1 = mRounded(od_int64,val1);
    if ( i1 == 0 )
	return mUdf(double);

    return (double)( i0 % i1 );
}


mMathExpressionClass( Abs, 1 )
double MathExpressionAbs::getValue() const
{
    return fabs(inputs_[0]->getValue());
}


mMathExpressionClass( Power, 2 )
double MathExpressionPower::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    if ( val0 < 0 )
    {
	int val1int = (int)val1;
	if ( !mIsEqual(val1,val1int,mDefEps) )
	    return mUdf(double);
    }

    return pow(val0,val1);
}


mMathExpressionClass( Condition, 3 )
double MathExpressionCondition::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    double val2 = inputs_[2]->getValue();
    if ( Values::isUdf(val0) )
	return mUdf(double);

    return !mIsZero(val0,mDefEps) ? val1 : val2;
}


mMathExpressionClass( LessOrEqual, 2 )
double MathExpressionLessOrEqual::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return val0 <= val1;
}


mMathExpressionClass( Less, 2 )
double MathExpressionLess::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return val0 < val1;
}


mMathExpressionClass( MoreOrEqual, 2 )
double MathExpressionMoreOrEqual::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return val0 >= val1;
}


mMathExpressionClass( More, 2 )
double MathExpressionMore::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return val0 > val1;
}


mMathExpressionClass( Equal, 2 )
double MathExpressionEqual::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();

    const bool val0udf = Values::isUdf(val0);
    const bool val1udf = Values::isUdf(val1);

    const int sum = val0udf + val1udf;
    if ( !sum )
	return mIsEqual(val0,val1,mDefEps);
    else if ( sum==2 )
	return true;

    return false;
}


mMathExpressionClass( NotEqual, 2 )
double MathExpressionNotEqual::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();

    const bool val0udf = Values::isUdf(val0);
    const bool val1udf = Values::isUdf(val1);

    const int sum = val0udf + val1udf;
    if ( !sum )
	return !mIsEqual(val0,val1,mDefEps);
    else if ( sum==2 )
	return false;

    return true;
}


mMathExpressionClass( OR, 2 )
double MathExpressionOR::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return !mIsZero(val0,mDefEps) || !mIsZero(val1,mDefEps);
}


mMathExpressionClass( AND, 2 )
double MathExpressionAND::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return !mIsZero(val0,mDefEps) && !mIsZero(val1,mDefEps);
}


static int ensureRandInited()
{
    Stats::randGen().init();
    return 0;
}

mMathExpressionClass( Random, 1 )
double MathExpressionRandom::getValue() const
{
    double maxval = inputs_[0]->getValue();
    if ( Values::isUdf(maxval) )
	return mUdf(double);

    mDefineStaticLocalObject( int, dum, mUnusedVar = ensureRandInited() );
    return ( double )( maxval * Stats::randGen().get() );
}


mMathExpressionClass( GaussRandom, 1 )
double MathExpressionGaussRandom::getValue() const
{
    const double stdev = inputs_[0]->getValue();
    if ( Values::isUdf(stdev) )
	return mUdf(double);

    mDefineStaticLocalObject( int, dum, mUnusedVar = ensureRandInited() );
    return ( double ) Stats::randGen().getNormal(0,stdev);
}


#define mMathExpressionOper( clss, func ) \
class MathExpression##clss : public MathExpression \
{ \
public: \
\
MathExpression##clss() \
    : MathExpression( 1 ) \
{} \
\
double getValue() const \
{ \
    double val = inputs_[0]->getValue(); \
    if ( Values::isUdf(val) ) \
	return mUdf(double); \
\
    double out = func(val); \
    return out == out ? out : mUdf(double); \
} \
\
MathExpression* clone() const \
{ \
    MathExpression* res = new MathExpression##clss(); \
    copyInput( res ); \
    return res; \
} \
 \
};


mMathExpressionOper( Sine, sin )
mMathExpressionOper( ArcSine, Math::ASin )
mMathExpressionOper( Cosine, cos )
mMathExpressionOper( ArcCosine, Math::ACos )
mMathExpressionOper( Tangent, tan )
mMathExpressionOper( ArcTangent, atan )
mMathExpressionOper( Log, Math::Log10 )
mMathExpressionOper( NatLog, Math::Log )
mMathExpressionOper( Exp, Math::Exp )
mMathExpressionOper( Sqrt, Math::Sqrt );


#define mMathExpressionStats( statnm ) \
class MathExpression##statnm : public MathExpression \
{ \
public: \
\
MathExpression##statnm( int nrvals ) \
    : MathExpression( nrvals ) \
{} \
\
double getValue() const \
{ \
    Stats::RunCalc<double> stats( \
	Stats::CalcSetup().require( Stats::statnm ) ); \
    for ( int idx=0; idx<inputs_.size(); idx++) \
	stats += inputs_[idx]->getValue(); \
\
    return (double)stats.getValue( Stats::statnm ); \
} \
\
MathExpression* clone() const \
{ \
    MathExpression* res = \
	new MathExpression##statnm( inputs_.size() ); \
    copyInput( res ); \
    return res; \
} \
\
};


mMathExpressionStats( Min );
mMathExpressionStats( Max );
mMathExpressionStats( Sum );
mMathExpressionStats( Median );
mMathExpressionStats( Average );
mMathExpressionStats( Variance );


const char* MathExpression::fullVariableExpression( int var ) const
{
    if ( var>=nrVariables() ) return 0;

    int input = (*variableobj_[var])[0];
    int v = (*variablenr_[var])[0];
    return inputs_[input]->fullVariableExpression( v );
}


void MathExpression::setVariableValue( int var, double val )
{
    if ( var>=nrVariables() || var<0 ) return;

    for ( int idx=0; idx<variableobj_[var]->size(); idx++ )
    {
	int input = (*variableobj_[var])[idx];
	int v = (*variablenr_[var])[idx];

	inputs_[input]->setVariableValue( v, val );
    }
}


bool MathExpression::setInput( int inp, MathExpression* obj )
{
    if ( inp>=0 && inp<nrInputs() )
    {
	if ( inputs_[inp] ) return false;
	delete inputs_.replace( inp, obj );

	for ( int idx=0; idx<obj->nrVariables(); idx++ )
	{
	    FixedString str = obj->fullVariableExpression(idx);

	    bool found=false;

	    for ( int idy=0; idy<nrVariables(); idy++ )
	    {
		if ( str==fullVariableExpression(idy) )
		{
		    (*variableobj_[idy]) += inp;
		    (*variablenr_[idy]) += idx;
		    found = true;
		    break;
		}
	    }

	    if ( !found )
	    {
		variableobj_ += new TypeSet<int>( 1, inp );
		variablenr_ += new TypeSet<int>( 1, idx );
		addIfOK( str );
	    }
	}

	return true;
    }

    return true;
}


MathExpression::VarType MathExpression::getType( int ivar ) const
{
    const BufferString varnm( MathExpressionParser::varNameOf(
			      fullVariableExpression(ivar) ) );
    return MathExpressionParser::varTypeOf( varnm.buf() );
}


int MathExpression::getConstIdx( int ivar ) const
{
    return MathExpressionParser::constIdxOf( fullVariableExpression(ivar) );
}


MathExpression::MathExpression( int sz )
    : isrecursive_(false)
{
    inputs_.allowNull();
    for ( int idx=0; idx<sz; idx++ )
	inputs_ += 0;
}


MathExpression::~MathExpression( )
{
    deepErase( inputs_ );
    deepErase( variableobj_ );
    deepErase( variablenr_ );
}


int MathExpression::nrVariables() const
{
    return variableobj_.size();
}


void MathExpression::copyInput( MathExpression* target ) const
{
    const int sz = nrInputs();

    for ( int idx=0; idx<sz; idx++ )
	target->setInput(idx, inputs_[idx]->clone() );
}


void MathExpression::addIfOK( const char* str )
{
    BufferString varnm = MathExpressionParser::varNameOf( str );
    if ( MathExpressionParser::varTypeOf( varnm ) == Recursive )
    {
	isrecursive_ = true;
	return;
    }

    varnms_.addIfNew( varnm );
}


int MathExpression::firstOccurVarName( const char* fullvnm ) const
{
    BufferString varnm = MathExpressionParser::varNameOf( fullvnm );
    for ( int idx=0; idx<nrVariables(); idx++ )
    {
	BufferString fullvar( fullVariableExpression(idx) );
	BufferString checkvarnm = MathExpressionParser::varNameOf( fullvar );
	if ( varnm == checkvarnm )
	    return idx;
    }

    return -1; //error, should never happen
}


const char* MathExpression::type() const
{
    return ::className(*this) + 14;
}


void MathExpression::doDump( BufferString& str, int nrtabs ) const
{
    const int nrvars = nrVariables();
    const int nrinps = inputs_.size();
    str.addTab( nrtabs ).add( type() );
    if ( isrecursive_ )
	str.add( " [recursive]" );
    str .add( ": " ).add( nrvars ).add( " vars" )
	.add( ", " ).add( nrinps ).add( " inps" ).addNewLine();
    dumpSpecifics( str, nrtabs );
    for ( int ivar=0; ivar<nrvars; ivar++ )
    {
	str.addTab( nrtabs ).add( "Var " ).add( ivar ).add( ": " )
	    .add( fullVariableExpression( ivar ) ).addNewLine();
    }
    for ( int iinp=0; iinp<nrinps; iinp++ )
	inputs_[iinp]->doDump( str, nrtabs+1 );
}


//--- Parser


BufferString MathExpressionParser::varNameOf( const char* str, int* shift )
{
    if ( shift ) *shift = 0;

    BufferString varnm( str );
    char* ptr = varnm.getCStr();
    while ( *ptr && *ptr != '[' ) ptr++;
    if ( !*ptr ) return varnm;
    *ptr++ = '\0';
    if ( shift )
    {
	const char* shftstr = ptr;
	while ( *ptr && *ptr != ']' ) ptr++;
	if ( *ptr ) *ptr = '\0';
	*shift = toInt( shftstr );
    }
    return varnm;
}


MathExpression::VarType MathExpressionParser::varTypeOf( const char* varnm )
{
    const BufferString vnm( varnm );

    if ( vnm.isEqual("this",CaseInsensitive)
      || vnm.isEqual("out",CaseInsensitive) )
	return MathExpression::Recursive;

    if ( vnm.size() > 1 && (vnm[0]=='c' || vnm[0]=='C') && isdigit(vnm[1]) )
	return MathExpression::Constant;

    return MathExpression::Variable;
}


int MathExpressionParser::constIdxOf( const char* varstr )
{
    if ( varTypeOf(varstr) != MathExpression::Constant )
	return -1;

    const BufferString varnm( varNameOf( varstr ) );
    const char* ptr = varnm.buf();
    while ( *ptr && !isdigit(*ptr) )
	ptr++;

    return toInt( ptr );
}


#define mErrRet(s) { errmsg_.set( s ); return true; }

#define mIfInAbsContinue() \
    if ( str[idx]=='|' && !(str[idx+1]=='|' || (idx && str[idx-1]=='|') ) ) \
	inabs = !inabs; \
    if ( inabs ) continue

#define mWithinParensContinue() \
	const char curch = str[idx]; \
	if ( curch == ')' ) parenslevel++; \
	else if ( curch == '(' ) parenslevel--; \
	if ( parenslevel ) continue


bool MathExpressionParser::findOuterParens( char* str, int len,
					      MathExpression*& ret ) const
{
    if ( str[0] != '(' || str[len-1] != ')' )
	return false;

    bool haveouterparens = true;
    int parenslevel = 0;
    for ( int idx=0; idx<len; idx++ )
    {
	const char curch = str[idx];
	if ( curch == '(' )
	    parenslevel++;
	else if ( curch == ')' )
	{
	    parenslevel--;
	    if ( parenslevel == 0 && haveouterparens )
		haveouterparens = idx == len-1;
	    else if ( parenslevel < 0 )
	    {
		str[idx+1] = '\0';
		errmsg_.set( "Found a closing parenthesis ')' too early" );
		if ( idx > 3 )
		    errmsg_.add( ": " ).add( str );
		return true;
	    }
	}
    }
    if ( parenslevel > 0 )
	mErrRet( "Not enough closing parentheses ')' found" );

    if ( haveouterparens )
    {
	str[len-1] = '\0';
	ret = parse( str+1 );
	return true;
    }

    return false;
}


bool MathExpressionParser::findOuterAbs( char* str, int len,
					      MathExpression*& ret ) const
{
    if ( str[0] != '|' || str[len-1] != '|' )
	return false;
    if ( len == 1 )
	mErrRet( "Single '|': invalid expression" );
    if ( len == 2 )
	mErrRet( "Stand-alone '||' found" );

    BufferString workstr( str+1 );
    workstr[len-2] = '\0';
    ret = parse( workstr );
    if ( ret )
    {
	MathExpression* absexp = new MathExpressionAbs;
	absexp->setInput( 0, ret );
	ret = absexp;
	abswarn_ = true;
	return true;
    }

    return false;
}


bool MathExpressionParser::findQMarkOper( char* str, int len,
					      MathExpression*& ret ) const
{
    bool inabs = false; int parenslevel = 0;
    int qmarkpos = -1;

    for ( int idx=0; idx<len; idx++ )
    {
	mIfInAbsContinue(); mWithinParensContinue();

	if ( curch == '?' )
	{
	    qmarkpos = idx;
	    if ( qmarkpos == 0 )
		mErrRet( "Please add a condition before '?'" )
	    continue;
	}
	else if ( curch == ':' )
	{
	    if ( qmarkpos < 0 )
		mErrRet( "Found ':' without '?'" )
	    if ( idx == qmarkpos+1 )
		mErrRet( "Please add a value for the 'true' condition" )
	    if ( idx == len - 1 )
		mErrRet( "Please add a value for the 'false' condition" )
	    str[qmarkpos] = str[idx] = '\0';
	    MathExpression* cond = parse( str );
	    if ( !cond )
		return true;
	    MathExpression* trueexp = parse( str + qmarkpos + 1 );
	    if ( !trueexp )
		{ delete cond; return true; }
	    MathExpression* falseexp = parse( str + idx + 1 );
	    if ( !falseexp )
		{ delete cond; delete trueexp; return 0; }
	    ret = new MathExpressionCondition;
	    ret->setInput( 0, cond );
	    ret->setInput( 1, trueexp );
	    ret->setInput( 2, falseexp );
	    return true;
	}
    }

    return false;
}


bool MathExpressionParser::findAndOrOr( char* str, int len,
					      MathExpression*& ret ) const
{
    bool inabs = false; int parenslevel = 0;

    for ( int idx=0; idx<len-1; idx++ )
    {
	mIfInAbsContinue(); mWithinParensContinue();

	if ( (str[idx]=='&'&&str[idx+1]=='&')||(str[idx]=='|'&&str[idx+1]=='|'))
	{
	    const bool isand = str[idx] == '&';
	    if ( idx == 0 )
		mErrRet( "No left-hand side for '&&' or '||' found" )
	    else if ( idx == len-2 )
		mErrRet( "No right-hand side for '&&' or '||' found" )

	    str[idx] = '\0';
	    MathExpression* inp0 = parse( str );
	    if ( !inp0 )
		return true;
	    MathExpression* inp1 = parse( str + idx + 2 );
	    if ( !inp1 )
		{ delete inp0; return true; }

	    if ( isand )
		ret = new MathExpressionAND;
	    else
		ret = new MathExpressionOR;
	    ret->setInput( 0, inp0 );
	    ret->setInput( 1, inp1 );
	    return true;
	}
    }

    return false;
}


bool MathExpressionParser::findInequality( char* str, int len,
					      MathExpression*& ret ) const
{
    bool inabs = false; int parenslevel = 0;

    for ( int idx=0; idx<len; idx++ )
    {
	mIfInAbsContinue(); mWithinParensContinue();

	const bool iseq = curch == '=';
	const bool islt = curch == '<';
	const bool isgt = curch == '>';
	const bool isnot = curch == '!';
	if ( iseq || islt || isgt || isnot )
	{
	    const char nextch = idx < len-1 ? str[idx+1] : ' ';
	    if ( (iseq || isnot) && nextch != '=' )
	    {
		str[idx+2] = '\0';
		errmsg_.set( "Invalid operator found: ").add( str+idx );
		return true;
	    }
	    if ( idx == 0 )
		mErrRet( "No left-hand side for (in-)equality operator found" )
	    else if ( idx == len-1 )
		mErrRet( "No right-hand side for (in-)equality operator found" )

	    str[idx] = '\0';
	    MathExpression* inp0 = parse( str );
	    if ( !inp0 )
		return true;
	    const int nrchars = nextch == '=' ? 2 : 1;
	    MathExpression* inp1 = parse( str + idx + nrchars );
	    if ( !inp1 )
		{ delete inp0; return true; }

	    if ( islt )
	    {
		if ( nextch == '=' )
		    ret = new MathExpressionLessOrEqual;
		else
		    ret = new MathExpressionLess;
	    }
	    else if ( isgt )
	    {
		if ( nextch == '=' )
		    ret = new MathExpressionMoreOrEqual;
		else
		    ret = new MathExpressionMore;
	    }
	    else if ( iseq )
		ret = new MathExpressionEqual;
	    else
		ret = new MathExpressionNotEqual;

	    ret->setInput( 0, inp0 );
	    ret->setInput( 1, inp1 );
	    return true;
	}
    }

    return false;
}


bool MathExpressionParser::findPlusAndMinus( char* str, int len,
					      MathExpression*& ret ) const
{
    bool inabs = false; int parenslevel = 0;

    for ( int idx=0; idx<len; idx++ )
    {
	mIfInAbsContinue(); mWithinParensContinue();

	if ( curch == '+' ||  curch == '-' )
	{
	    if ( idx == 0 || idx == len-1 )
		continue;
	    const char prevch = str[idx-1];
	    if ( prevch == '(' || prevch == '+' || prevch == '-'
		|| prevch == '*' || prevch == '/' || prevch == '/'
		|| prevch == '^' || prevch == '[' )
		continue;
	    if ( idx > 1 && (prevch == 'e' || prevch == 'E')
		&& !isalpha(str[idx-2]) )
		continue;

	    MathExpression* inp1 = parse( curch == '+' ? str+idx+1 : str+idx );
	    if ( !inp1 )
		return true;
	    str[idx] = '\0';
	    MathExpression* inp0 = parse( str );
	    if ( !inp0 )
		{ delete inp1; return true; }

	    ret = new MathExpressionPlus;
	    ret->setInput( 0, inp0 );
	    ret->setInput( 1, inp1 );
	    return true;
	}
    }

    // unary '-'
    if ( str[0]== '-' )
    {
	MathExpression* inp0 = new MathExpressionConstant( -1 );
	MathExpression* inp1 = parse( str+1 );
	if ( !inp1 )
	    return true;
	ret = new MathExpressionMultiply;
	ret->setInput( 0, inp0 );
	ret->setInput( 1, inp1 );
	return true;
    }

    return false;
}


bool MathExpressionParser::findOtherOper( BufferString& workstr, int len,
					      MathExpression*& ret ) const
{
    char* str = workstr.getCStr();
    bool inabs = false; int parenslevel = 0;

#define mParseOperator( op, clss ) \
    inabs = false; parenslevel = 0; \
    for ( int idx=0; idx<len; idx++ ) \
    { \
	mIfInAbsContinue(); mWithinParensContinue(); \
	if ( curch == op ) \
	{ \
	    if ( idx == 0 || idx == len-1 ) \
		continue; \
	    str[idx] = '\0'; \
	    MathExpression* inp0 = parse( str ); \
	    if ( !inp0 ) return true; \
	    MathExpression* inp1 = parse( str+idx+1 ); \
	    if ( !inp1 ) \
		{ delete inp0; return true; } \
	    ret = new MathExpression##clss; \
	    ret->setInput( 0, inp0 ); \
	    ret->setInput( 1, inp1 ); \
	    return true; \
	} \
    }

    mParseOperator( '*', Multiply );
    mParseOperator( '/', Divide );
    mParseOperator( '^', Power );
    mParseOperator( '%', IntDivRest );

    return false;
}


static char* findLooseComma( char* str )
{
    bool inabs = false; int parenslevel = 0;
    int idx = 0;
    while ( *str )
    {
	mIfInAbsContinue(); mWithinParensContinue();
	if ( *str == ',' )
	    return str;
	idx++; str++;
    }
    return 0;
}


bool MathExpressionParser::findMathFunction( BufferString& workstr, int len,
					      MathExpression*& ret ) const
{
    char* str = workstr.getCStr();
#   define mParseFunction( nm, clss ) { \
    if ( workstr.startsWith( #nm "(", CaseInsensitive ) ) \
    { \
	workstr[len-1] = '\0'; \
	MathExpression* inp = parse( str + FixedString( #nm "(" ).size() ); \
	if ( !inp ) return true; \
	ret = new MathExpression##clss; \
	ret->setInput( 0, inp ); \
	return true; \
    } \
    }
    mParseFunction( abs, Abs )
    mParseFunction( sqrt, Sqrt )
    mParseFunction( exp, Exp )

    mParseFunction( ln, NatLog )
    mParseFunction( log, Log )

    mParseFunction( sin, Sine )
    mParseFunction( asin, ArcSine )
    mParseFunction( cos, Cosine )
    mParseFunction( acos, ArcCosine )
    mParseFunction( tan, Tangent )
    mParseFunction( atan, ArcTangent )

    mParseFunction( rand, Random )
    mParseFunction( randg, GaussRandom )

#   define mParse2ArgsFunction( nm, clss ) { \
    if ( workstr.startsWith( #nm "(", CaseInsensitive ) ) \
    { \
	workstr[len-1] = '\0'; \
	const int fnnameskipsz = FixedString( #nm ).size() + 1; \
	char* ptrcomma = findLooseComma( str+fnnameskipsz ); \
	if ( !ptrcomma ) \
	    mErrRet( #nm " function takes 2 arguments" ) \
	*ptrcomma++ = '\0'; \
	MathExpression* inp0 = parse( str + fnnameskipsz ); \
	MathExpression* inp1 = parse( ptrcomma ); \
	if ( !inp0 || !inp1 ) return true; \
	ret = new MathExpression##clss; \
	ret->setInput( 0, inp0 ); \
	ret->setInput( 1, inp1 ); \
	return true; \
    } \
    }

    mParse2ArgsFunction( idiv, IntDivide )
    mParse2ArgsFunction( mod, IntDivRest )

    return false;
}


bool MathExpressionParser::findStatsFunction( BufferString& workstr, int len,
					      MathExpression*& ret ) const
{
    char* str = workstr.getCStr();
#   define mGetIsFnMatch(nm) \
    const bool is##nm = workstr.startsWith( #nm "(", CaseInsensitive )

    mGetIsFnMatch(min); mGetIsFnMatch(max); mGetIsFnMatch(sum);
    mGetIsFnMatch(med); mGetIsFnMatch(var); mGetIsFnMatch(avg);

    if ( ismin || ismax || issum || ismed || isvar || isavg )
    {
	BufferStringSet args;
	int argstart = 4;
	TypeSet<int> argumentstop;
	bool inabs = false; int parenslevel = 0;
	for ( int idx=argstart; idx<len; idx++ )
	{
	    mIfInAbsContinue();
	    if ( idx != len-1 )
		{ mWithinParensContinue(); }

	    if ( idx == len-1 || str[idx] == ',' )
	    {
		if ( idx == argstart )
		    mErrRet("Empty argument(s) to statistical funcion")

		str[idx] = '\0';
		args.add( str + argstart );
		argstart = idx + 1;
	    }
	}

	ObjectSet<MathExpression> inputs_;
	for ( int idx=0; idx<args.size(); idx++ )
	{
	    MathExpression* inp = parse( args.get(idx) );
	    if ( !inp )
	    {
		deepErase( inputs_ );
		return true;
	    }

	    inputs_ += inp;
	}

	const int inpssz = inputs_.size();
	if ( ismax )
	    ret = new MathExpressionMax( inpssz );
	else if ( ismin )
	    ret = new MathExpressionMin( inpssz );
	else if ( issum )
	    ret = new MathExpressionSum( inpssz );
	else if ( ismed )
	    ret = new MathExpressionMedian( inpssz );
	else if ( isavg )
	    ret = new MathExpressionAverage( inpssz );
	else if ( isvar )
	    ret = new MathExpressionVariance( inpssz );
	if ( !ret )
	    return true;

	for ( int idx=0; idx<inpssz; idx++ )
	    ret->setInput( idx, inputs_[idx] );
	return true;
    }

    return false;
}


bool MathExpressionParser::findVariable( char* str, int len,
					      MathExpression*& ret ) const
{
    bool isvariable = true;
    for ( int idx=0; idx<len; idx++ )
    {
	if ( (!idx&&isdigit(str[idx])) || !isalnum(str[idx]) )
	{
	    isvariable = str[idx] == '_' || str[idx] == '[' || str[idx] == ']';
	    break;
	}
    }
    if ( isvariable )
    {
	if ( varTypeOf(str)== MathExpression::Recursive )
	{
	    int recshift; varNameOf( str, &recshift );
	    if ( recshift >= 0 )
	    {
		errmsg_.set( "Invalid recursive expression:\n'" )
		       .add( str ).add( "'\nRecursive expression must be "
					"of type 'THIS[-n]' or 'OUT[-n]'" );
		return true;
	    }
	}

	ret = new MathExpressionVariable( str );
	return true;
    }

    return false;
}


MathExpression* MathExpressionParser::parse( const char* inpstr ) const
{
    errmsg_.setEmpty();
    if ( FixedString(inpstr).isEmpty() )
	return 0;

    BufferString workstr( inpstr );
    workstr.remove( ' ' );
    const int len = workstr.size();
    if ( len < 1 )
	return 0;
    char* str = workstr.getCStr();
    MathExpression* ret = 0;

    if ( findOuterParens( str, len, ret ) )
	return ret;
    else if ( findQMarkOper( str, len, ret ) )
	return ret;
    else if ( findAndOrOr( str, len, ret ) )
	return ret;
    else if ( findInequality( str, len, ret ) )
	return ret;
    else if ( findPlusAndMinus( str, len, ret ) )
	return ret;
    else if ( findOtherOper( workstr, len, ret ) )
	return ret;
    else if ( findOuterAbs( str, len, ret ) )
	return ret;

    double dres = mUdf(double);
    if ( getFromString(dres,str,mUdf(float)) )
	return new MathExpressionConstant( dres );

    if ( str[len-1] == ')' )
    {
	if ( findMathFunction( workstr, len, ret ) )
	    return ret;
	else if ( findStatsFunction( workstr, len, ret ) )
	    return ret;
    }

    if ( workstr.isEqual("pi",CaseInsensitive) )
	return new MathExpressionConstant( M_PI );
    else if ( workstr.isEqual("undef",CaseInsensitive) )
	return new MathExpressionConstant( mUdf(double) );

    if ( findVariable( str, len, ret ) )
	return ret;

    errmsg_.set( "Cannot parse this:\n'" ).add( inpstr ).add( "'" );
    return 0;
}


MathExpression* MathExpressionParser::parse() const
{
    if ( inp_.isEmpty() )
	{ errmsg_ = "Empty input"; return 0; }
    return parse( inp_.buf() );
}
