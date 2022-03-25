/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Mar 2000
________________________________________________________________________

-*/

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


const ObjectSet<const Math::ExpressionOperatorDescGroup>&
			Math::ExpressionOperatorDescGroup::supported()
{
    mDefineStaticLocalObject(
	PtrMan<ManagedObjectSet<const Math::ExpressionOperatorDescGroup> >,
	ret, = 0 );

    if ( ret ) return *ret;
    ret = new ManagedObjectSet<const Math::ExpressionOperatorDescGroup>;

    Math::ExpressionOperatorDescGroup* grp
				= new Math::ExpressionOperatorDescGroup;
    grp->name_ = "Basic";

#   define mAddDesc(s,d,i,n) \
    grp->opers_ += new Math::ExpressionOperatorDesc(s,d,i,n)
    mAddDesc("+","Add",true,2);
    mAddDesc("-","Subtract",true,2);
    mAddDesc("*","Multiply",true,2);
    mAddDesc("/","Divide",true,2);
    mAddDesc("^","Power",true,2);
    *ret += grp;

    grp = new Math::ExpressionOperatorDescGroup;
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

    grp = new Math::ExpressionOperatorDescGroup;
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

    grp = new Math::ExpressionOperatorDescGroup;
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

    grp = new Math::ExpressionOperatorDescGroup;
    grp->name_ = "Constants";
    mAddDesc("pi","PI=3.14...",true,0);
    mAddDesc("euler","e=2.71...",true,0);
    mAddDesc("undef","Undefined",true,0);
    *ret += grp;

    return *ret;
}



namespace Math
{

class ExpressionVariable : public Expression
{
public:
				ExpressionVariable( const char* str )
				    : Expression( 0 )
				    , str_(str) { addIfOK(str_.buf()); }

    const char*			fullVariableExpression( int ) const
							{ return str_.buf(); }

    int	nrVariables() const	{ return 1; }

    double			getValue() const	{ return val_; }

    void			setVariableValue( int, double nv )
							{ val_ = nv; }

    Expression*		clone() const
				{
				    Expression* res =
					new ExpressionVariable(str_.buf());
				    copyInput( res );
				    return res;
				}

protected:

    double			val_;
    BufferString		str_;

};



class ExpressionConstant : public Expression
{
public:

ExpressionConstant( double val )
    : val_ ( val )
    , Expression( 0 )
{
}

double getValue() const
{
    return val_;
}

Expression* clone() const
{
    Expression* res = new ExpressionConstant(val_);
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
class Expression##clss : public Expression \
{ \
public: \
Expression##clss() : Expression( nr ) \
{ \
} \
 \
double getValue() const; \
 \
Expression* clone() const \
{ \
    Expression* res = new Expression##clss(); \
    copyInput( res ); \
    return res; \
} \
 \
};


mMathExpressionClass( Plus, 2 )
double ExpressionPlus::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);
    return val0+val1;
}


mMathExpressionClass( Minus, 2 )
double ExpressionMinus::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return val0-val1;
}


mMathExpressionClass( Multiply, 2 )
double ExpressionMultiply::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return val0*val1;
}


mMathExpressionClass( Divide, 2 )
double ExpressionDivide::getValue() const
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
double ExpressionIntDivide::getValue() const
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
double ExpressionIntDivRest::getValue() const
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
double ExpressionAbs::getValue() const
{
    return fabs(inputs_[0]->getValue());
}


mMathExpressionClass( Power, 2 )
double ExpressionPower::getValue() const
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
double ExpressionCondition::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    double val2 = inputs_[2]->getValue();
    if ( Values::isUdf(val0) )
	return mUdf(double);

    return !mIsZero(val0,mDefEps) ? val1 : val2;
}


mMathExpressionClass( LessOrEqual, 2 )
double ExpressionLessOrEqual::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return val0 <= val1;
}


mMathExpressionClass( Less, 2 )
double ExpressionLess::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return val0 < val1;
}


mMathExpressionClass( MoreOrEqual, 2 )
double ExpressionMoreOrEqual::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return val0 >= val1;
}


mMathExpressionClass( More, 2 )
double ExpressionMore::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return val0 > val1;
}


mMathExpressionClass( Equal, 2 )
double ExpressionEqual::getValue() const
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
double ExpressionNotEqual::getValue() const
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
double ExpressionOR::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return !mIsZero(val0,mDefEps) || !mIsZero(val1,mDefEps);
}


mMathExpressionClass( AND, 2 )
double ExpressionAND::getValue() const
{
    double val0 = inputs_[0]->getValue();
    double val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(double);

    return !mIsZero(val0,mDefEps) && !mIsZero(val1,mDefEps);
}


mMathExpressionClass( Random, 1 )
double ExpressionRandom::getValue() const
{
    const double maxval = inputs_[0]->getValue();
    if ( Values::isUdf(maxval) )
	return mUdf(double);

    static Stats::RandGen gen;
    return maxval * gen.get();
}


mMathExpressionClass( GaussRandom, 1 )
double ExpressionGaussRandom::getValue() const
{
    const double stdev = inputs_[0]->getValue();
    if ( Values::isUdf(stdev) )
	return mUdf(double);

    static Stats::NormalRandGen rg;
    return stdev * rg.get();
}


#define mMathExpressionOper( clss, func ) \
class Expression##clss : public Expression \
{ \
public: \
\
Expression##clss() \
    : Expression( 1 ) \
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
Expression* clone() const \
{ \
    Expression* res = new Expression##clss(); \
    copyInput( res ); \
    return res; \
} \
 \
};


mMathExpressionOper( Sine, sin )
mMathExpressionOper( ArcSine, ASin )
mMathExpressionOper( Cosine, cos )
mMathExpressionOper( ArcCosine, ACos )
mMathExpressionOper( Tangent, tan )
mMathExpressionOper( ArcTangent, atan )
mMathExpressionOper( Log, Log10 )
mMathExpressionOper( NatLog, Log )
mMathExpressionOper( Exp, Exp )
mMathExpressionOper( Sqrt, Sqrt );


#define mMathExpressionStats( statnm ) \
class Expression##statnm : public Expression \
{ \
public: \
\
Expression##statnm( int nrvals ) \
    : Expression( nrvals ) \
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
Expression* clone() const \
{ \
    Expression* res = new Expression##statnm( inputs_.size() ); \
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

} // namespace Math


const char* Math::Expression::fullVariableExpression( int var ) const
{
    if ( var>=nrVariables() ) return 0;

    int input = (*variableobj_[var])[0];
    int v = (*variablenr_[var])[0];
    return inputs_[input]->fullVariableExpression( v );
}


void Math::Expression::setVariableValue( int var, double val )
{
    if ( var>=nrVariables() || var<0 ) return;

    for ( int idx=0; idx<variableobj_[var]->size(); idx++ )
    {
	int input = (*variableobj_[var])[idx];
	int v = (*variablenr_[var])[idx];

	inputs_[input]->setVariableValue( v, val );
    }
}


bool Math::Expression::setInput( int inp, Math::Expression* obj )
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


Math::Expression::VarType Math::Expression::getType( int ivar ) const
{
    const BufferString varnm( Math::ExpressionParser::varNameOf(
			      fullVariableExpression(ivar) ) );
    return Math::ExpressionParser::varTypeOf( varnm.buf() );
}


int Math::Expression::getConstIdx( int ivar ) const
{
    return Math::ExpressionParser::constIdxOf( fullVariableExpression(ivar) );
}


Math::Expression::Expression( int sz )
    : isrecursive_(false)
{
    inputs_.allowNull();
    for ( int idx=0; idx<sz; idx++ )
	inputs_ += 0;
}


Math::Expression::~Expression( )
{
    deepErase( inputs_ );
    deepErase( variableobj_ );
    deepErase( variablenr_ );
}


int Math::Expression::nrVariables() const
{
    return variableobj_.size();
}


void Math::Expression::copyInput( Math::Expression* target ) const
{
    const int sz = nrInputs();

    for ( int idx=0; idx<sz; idx++ )
	target->setInput(idx, inputs_[idx]->clone() );
}


void Math::Expression::addIfOK( const char* str )
{
    BufferString varnm = Math::ExpressionParser::varNameOf( str );
    if ( Math::ExpressionParser::varTypeOf( varnm ) == Recursive )
    {
	isrecursive_ = true;
	return;
    }

    varnms_.addIfNew( varnm );
}


int Math::Expression::firstOccurVarName( const char* fullvnm ) const
{
    BufferString varnm = Math::ExpressionParser::varNameOf( fullvnm );
    for ( int idx=0; idx<nrVariables(); idx++ )
    {
	BufferString fullvar( fullVariableExpression(idx) );
	BufferString checkvarnm = Math::ExpressionParser::varNameOf( fullvar );
	if ( varnm == checkvarnm )
	    return idx;
    }

    return -1; //error, should never happen
}


const char* Math::Expression::type() const
{
    return ::className(*this) + 14;
}


void Math::Expression::doDump( BufferString& str, int nrtabs ) const
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


BufferString Math::ExpressionParser::varNameOf( const char* str, int* shift )
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


Math::Expression::VarType Math::ExpressionParser::varTypeOf( const char* varnm )
{
    const BufferString vnm( varnm );

    if ( vnm.isEqual("this",CaseInsensitive)
      || vnm.isEqual("out",CaseInsensitive) )
	return Math::Expression::Recursive;

    if ( vnm.size() > 1 && (vnm[0]=='c' || vnm[0]=='C') && iswdigit(vnm[1]) )
	return Math::Expression::Constant;

    return Math::Expression::Variable;
}


int Math::ExpressionParser::constIdxOf( const char* varstr )
{
    if ( varTypeOf(varstr) != Math::Expression::Constant )
	return -1;

    const BufferString varnm( varNameOf( varstr ) );
    const char* ptr = varnm.buf();
    while ( *ptr && !iswdigit(*ptr) )
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


bool Math::ExpressionParser::findOuterParens( char* str, int len,
					      Math::Expression*& ret ) const
{
    bool haveouterparens = str[0] == '(' && str[len-1] == ')';
    int parenslevel = 0;
    int firstparenpos = -1;
    for ( int idx=0; idx<len; idx++ )
    {
	const char curch = str[idx];
	if ( curch == '(' )
	{
	    parenslevel++;
	    if ( firstparenpos < 0 )
		firstparenpos = idx;
	}
	else if ( curch == ')' )
	{
	    parenslevel--;
	    if ( parenslevel == 0 && haveouterparens )
		haveouterparens = idx == len-1;
	    else if ( parenslevel < 0 )
	    {
		str[idx+1] = '\0';
		errmsg_.set( "Found a closing parenthesis ')' "
			     "without a matching opening parenthesis '('" );
		if ( idx > 1 )
		    errmsg_.add( ": " ).add( str );
		return true;
	    }
	}
    }

    if ( parenslevel > 0 )
    {
	errmsg_.set( "Not enough closing parentheses ')' found" );
	if ( firstparenpos >= 0 )
	    errmsg_.add( ": " ).add( str + firstparenpos );
	return true;
    }

    if ( haveouterparens )
    {
	str[len-1] = '\0';
	ret = parse( str+1 );
	return true;
    }

    return false;
}


bool Math::ExpressionParser::findOuterAbs( char* str, int len,
					      Math::Expression*& ret ) const
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
	Math::Expression* absexp = new Math::ExpressionAbs;
	absexp->setInput( 0, ret );
	ret = absexp;
	abswarn_ = true;
	return true;
    }

    return false;
}


bool Math::ExpressionParser::findQMarkOper( char* str, int len,
					      Math::Expression*& ret ) const
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
	    PtrMan<Math::Expression> cond = parse( str );
	    if ( !cond )
		return true;
	    PtrMan<Math::Expression> trueexp = parse( str + qmarkpos + 1 );
	    if ( !trueexp )
		{ return true; }
	    PtrMan<Math::Expression> falseexp = parse( str + idx + 1 );
	    if ( !falseexp )
		{ return 0; }
	    ret = new Math::ExpressionCondition;
	    ret->setInput( 0, cond.release() );
	    ret->setInput( 1, trueexp.release() );
	    ret->setInput( 2, falseexp.release() );
	    return true;
	}
    }

    return false;
}


bool Math::ExpressionParser::findAndOrOr( char* str, int len,
					      Math::Expression*& ret ) const
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
	    PtrMan<Math::Expression> inp0 = parse( str );
	    if ( !inp0 )
		return true;
	    PtrMan<Math::Expression> inp1 = parse( str + idx + 2 );
	    if ( !inp1 )
		return true;

	    if ( isand )
		ret = new Math::ExpressionAND;
	    else
		ret = new Math::ExpressionOR;
	    ret->setInput( 0, inp0.release() );
	    ret->setInput( 1, inp1.release() );
	    return true;
	}
    }

    return false;
}


bool Math::ExpressionParser::findInequality( char* str, int len,
					      Math::Expression*& ret ) const
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
	    PtrMan<Math::Expression> inp0 = parse( str );
	    if ( !inp0 )
		return true;
	    const int nrchars = nextch == '=' ? 2 : 1;
	    PtrMan<Math::Expression> inp1 = parse( str + idx + nrchars );
	    if ( !inp1 )
		{ return true; }

	    if ( islt )
	    {
		if ( nextch == '=' )
		    ret = new Math::ExpressionLessOrEqual;
		else
		    ret = new Math::ExpressionLess;
	    }
	    else if ( isgt )
	    {
		if ( nextch == '=' )
		    ret = new Math::ExpressionMoreOrEqual;
		else
		    ret = new Math::ExpressionMore;
	    }
	    else if ( iseq )
		ret = new Math::ExpressionEqual;
	    else
		ret = new Math::ExpressionNotEqual;

	    ret->setInput( 0, inp0.release() );
	    ret->setInput( 1, inp1.release() );
	    return true;
	}
    }

    return false;
}


bool Math::ExpressionParser::findPlusAndMinus( char* str, int len,
					      Math::Expression*& ret ) const
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
		&& !iswalpha(str[idx-2]) )
		continue;

	    PtrMan<Math::Expression> inp1
			= parse( curch == '+' ? str+idx+1 : str+idx );
	    if ( !inp1 )
		return true;
	    str[idx] = '\0';
	    PtrMan<Math::Expression> inp0 = parse( str );
	    if ( !inp0 )
		return true;

	    ret = new Math::ExpressionPlus;
	    ret->setInput( 0, inp0.release() );
	    ret->setInput( 1, inp1.release() );
	    return true;
	}
    }

    // unary '-'
    if ( str[0]== '-' )
    {
	PtrMan<Math::Expression> inp0 = new Math::ExpressionConstant( -1 );
	PtrMan<Math::Expression> inp1 = parse( str+1 );
	if ( !inp1 )
	    return true;

	ret = new Math::ExpressionMultiply;
	ret->setInput( 0, inp0.release() );
	ret->setInput( 1, inp1.release() );
	return true;
    }

    return false;
}


bool Math::ExpressionParser::findOtherOper( BufferString& workstr, int len,
					      Math::Expression*& ret ) const
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
	    PtrMan<Math::Expression> inp0 = parse( str ); \
	    if ( !inp0 ) return true; \
	    PtrMan<Math::Expression> inp1 = parse( str+idx+1 ); \
	    if ( !inp1 ) return true; \
	    ret = new Math::Expression##clss; \
	    ret->setInput( 0, inp0.release() ); \
	    ret->setInput( 1, inp1.release() ); \
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


bool Math::ExpressionParser::findMathFunction( BufferString& workstr, int len,
					      Math::Expression*& ret ) const
{
    char* str = workstr.getCStr();
#   define mParseFunction( nm, clss ) { \
    if ( workstr.startsWith( #nm "(", CaseInsensitive ) ) \
    { \
	workstr[len-1] = '\0'; \
	PtrMan<Math::Expression> inp = \
		parse( str + FixedString( #nm "(" ).size() ); \
	if ( !inp ) return true; \
	ret = new Math::Expression##clss; \
	ret->setInput( 0, inp.release() ); \
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
	PtrMan<Math::Expression> inp0 = parse( str + fnnameskipsz ); \
	PtrMan<Math::Expression> inp1 = parse( ptrcomma ); \
	if ( !inp0 || !inp1 ) return true; \
	ret = new Math::Expression##clss; \
	ret->setInput( 0, inp0.release() ); \
	ret->setInput( 1, inp1.release() ); \
	return true; \
    } \
    }

    mParse2ArgsFunction( idiv, IntDivide )
    mParse2ArgsFunction( mod, IntDivRest )

    return false;
}


bool Math::ExpressionParser::findStatsFunction( BufferString& workstr, int len,
					      Math::Expression*& ret ) const
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

	ObjectSet<Math::Expression> inputs;
	for ( int idx=0; idx<args.size(); idx++ )
	{
	    Math::Expression* inp = parse( args.get(idx) );
	    if ( !inp )
	    {
		deepErase( inputs );
		return true;
	    }

	    inputs += inp;
	}

	const int inpssz = inputs.size();
	if ( ismax )
	    ret = new Math::ExpressionMax( inpssz );
	else if ( ismin )
	    ret = new Math::ExpressionMin( inpssz );
	else if ( issum )
	    ret = new Math::ExpressionSum( inpssz );
	else if ( ismed )
	    ret = new Math::ExpressionMedian( inpssz );
	else if ( isavg )
	    ret = new Math::ExpressionAverage( inpssz );
	else if ( isvar )
	    ret = new Math::ExpressionVariance( inpssz );
	if ( !ret )
	    return true;

	for ( int idx=0; idx<inpssz; idx++ )
	    ret->setInput( idx, inputs[idx] );
	return true;
    }

    return false;
}


bool Math::ExpressionParser::findVariable( char* str, int len,
					      Math::Expression*& ret ) const
{
    bool isok = true;
    int distfromopeningbracket = -1;
    bool hasvaridx = false;
    for ( int idx=0; idx<len; idx++ )
    {
	if ( hasvaridx )
	    return false; // there are chars after []

	if ( distfromopeningbracket < 0 )
	{
	    isok = idx == 0 ? iswalpha( str[idx] ) : iswalnum( str[idx] );
	    if ( !isok && str[idx] == '_' )
		isok = true;
	    if ( !isok )
	    {
		if ( str[idx] != '['  )
		    return false;
		distfromopeningbracket = 0;
		isok = true;
	    }
	}
	else
	{
	    isok = iswdigit( str[idx] );
	    if ( !isok )
	    {
		if ( distfromopeningbracket == 0 )
		    isok = str[idx] == '+' || str[idx] == '-';
		else if ( str[idx] == ']' )
		    { hasvaridx = true; isok = true; }
	    }
	    distfromopeningbracket++;
	}
	if ( !isok )
	    return false;
    }

    if ( hasvaridx && !inputsareseries_ )
    {
	errmsg_.set( "Found recursive or shifted variables."
		"\nBut the formula is not for data series" );
	return false;
    }

    ret = new Math::ExpressionVariable( str );
    return true;
}


Math::Expression* Math::ExpressionParser::parse( const char* inpstr ) const
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
    Math::Expression* ret = 0;

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
	return new Math::ExpressionConstant( dres );

    if ( str[len-1] == ')' )
    {
	if ( findMathFunction( workstr, len, ret ) )
	    return ret;
	else if ( findStatsFunction( workstr, len, ret ) )
	    return ret;
    }

    if ( workstr.isEqual("pi",CaseInsensitive) )
	return new Math::ExpressionConstant( M_PI );
    if ( workstr.isEqual("euler",CaseInsensitive) )
	return new Math::ExpressionConstant( 2.7182818284590452353602874713 );
    if ( workstr.isEqual("undef",CaseInsensitive) )
	return new Math::ExpressionConstant( mUdf(double) );
    if ( workstr.isEqual("null",CaseInsensitive) )
	return new Math::ExpressionConstant( mUdf(double) );

    if ( findVariable( str, len, ret ) )
	return ret;

    if ( errmsg_.isEmpty() )
	errmsg_.set( "Cannot parse this:\n'" ).add( inpstr ).add( "'" );
    return 0;
}


Math::Expression* Math::ExpressionParser::parse() const
{
    if ( inp_.isEmpty() )
	{ errmsg_ = "Empty input"; return 0; }

    return parse( inp_.buf() );
}
