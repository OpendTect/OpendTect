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

#include <math.h>

#define absolute( str, idx, inabs) \
    if ( str[idx]=='|' && !(str[idx+1]=='|' || (idx && str[idx-1]=='|') ) ) \
	inabs=(inabs+1)%2;

#ifndef M_PI
# define M_PI           3.14159265358979323846  /* pi */
#endif


#define mAddDesc(s,d,i,n) \
    grp->opers_ += new MathExpressionOperatorDesc(s,d,i,n)

const ObjectSet<const MathExpressionOperatorDescGroup>&
			MathExpressionOperatorDescGroup::supported()
{
    static ObjectSet<const MathExpressionOperatorDescGroup>* ret = 0;
    if ( ret ) return *ret;
    ret = new ObjectSet<const MathExpressionOperatorDescGroup>;

    MathExpressionOperatorDescGroup* grp = new MathExpressionOperatorDescGroup;
    grp->name_ = "Basic";
    mAddDesc("+","Add",true,2);
    mAddDesc("-","Subtract",true,2);
    mAddDesc("*","Multiply",true,2);
    mAddDesc("/","Divide",true,2);
    mAddDesc("^","Power",true,2);
    mAddDesc("|","Integer division",true,2);
    mAddDesc("%","Rest after integer division",true,2);
    mAddDesc("| |","Absolute value",true,1);
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
				    , str_( new char[strlen(str)+1] )
				{ strcpy( str_, str ); addIfOK(str); }

				~MathExpressionVariable() { delete [] str_; }

    const char*			fullVariableExpression( int ) const
				{ return str_; }

    int				nrVariables() const { return 1; }

    float			getValue() const
				{
				    return val_;
				}

    void			setVariableValue( int, float nv )
				{ val_ = nv; }

    MathExpression*		clone() const
				{
				    MathExpression* res =
					new MathExpressionVariable(str_);
				    copyInput( res );
				    return res;
				}


protected:
    float 			val_;
    char*			str_;
};


class MathExpressionConstant : public MathExpression
{
public:
				MathExpressionConstant( float val )
				    : val_ ( val )
				    , MathExpression( 0 )		{}

    float			getValue() const { return val_; }

    MathExpression*		clone() const
				{
				    MathExpression* res =
					new MathExpressionConstant(val_);
				    copyInput( res );
				    return res;
				}

protected:
    float 			val_;
};


#define mMathExpressionClass( clss, nr ) \
class MathExpression##clss : public MathExpression \
{ \
public: \
			MathExpression##clss() \
			    : MathExpression( nr )		{} \
 \
    float		getValue() const; \
 \
    MathExpression*	clone() const \
			{ \
			    MathExpression* res = \
					new MathExpression##clss(); \
			    copyInput( res ); \
			    return res; \
			} \
};


mMathExpressionClass( Plus, 2 )
float MathExpressionPlus::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(float);
    return val0+val1;
}


mMathExpressionClass( Minus, 2 )
float MathExpressionMinus::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(float);

    return val0-val1;
}


mMathExpressionClass( Multiply, 2 )
float MathExpressionMultiply::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(float);

    return val0*val1;
}


mMathExpressionClass( Divide, 2 )
float MathExpressionDivide::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) || mIsZero(val1,mDefEps) )
	return mUdf(float);

    return val0/val1;
}


mMathExpressionClass( IntDivide, 2 )
float MathExpressionIntDivide::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(float);

    const od_int64 i0 = mRounded(od_int64,val0);
    const od_int64 i1 = mRounded(od_int64,val1);
    if ( i1 == 0 )
	return mUdf(float);

    return (float)( i0 / i1 );
}


mMathExpressionClass( IntDivRest, 2 )
float MathExpressionIntDivRest::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(float);

    const od_int64 i0 = mRounded(od_int64,val0);
    const od_int64 i1 = mRounded(od_int64,val1);
    if ( i1 == 0 )
	return mUdf(float);

    return (float)( i0 % i1 );
}


mMathExpressionClass( Abs, 1 )
float MathExpressionAbs::getValue() const
{
    return fabs(inputs_[0]->getValue());
}


mMathExpressionClass( Power, 2 )
float MathExpressionPower::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(float);

    if ( val0 < 0 )
    {
	int val1int = (int)val1;
	if ( !mIsEqual(val1,val1int,mDefEps) )
	    return mUdf(float);
    }

    return pow(val0,val1);
}


mMathExpressionClass( Condition, 3 )
float MathExpressionCondition::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();
    float val2 = inputs_[2]->getValue();
    if ( Values::isUdf(val0) )
	return mUdf(float);

    return !mIsZero(val0,mDefEps) ? val1 : val2;
}


mMathExpressionClass( LessOrEqual, 2 )
float MathExpressionLessOrEqual::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(float);

    return val0 <= val1;
}


mMathExpressionClass( Less, 2 )
float MathExpressionLess::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(float);

    return val0 < val1;
}


mMathExpressionClass( MoreOrEqual, 2 )
float MathExpressionMoreOrEqual::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(float);

    return val0 >= val1;
}


mMathExpressionClass( More, 2 )
float MathExpressionMore::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(float);

    return val0 > val1;
}


mMathExpressionClass( Equal, 2 )
float MathExpressionEqual::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();

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
float MathExpressionNotEqual::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();

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
float MathExpressionOR::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(float);

    return !mIsZero(val0,mDefEps) || !mIsZero(val1,mDefEps);
}


mMathExpressionClass( AND, 2 )
float MathExpressionAND::getValue() const
{
    float val0 = inputs_[0]->getValue();
    float val1 = inputs_[1]->getValue();
    if ( Values::isUdf(val0) || Values::isUdf(val1) )
	return mUdf(float);

    return !mIsZero(val0,mDefEps) && !mIsZero(val1,mDefEps);
}


static int ensureRandInited()
{
    Stats::RandGen::init();
    return 0;
}

mMathExpressionClass( Random, 1 )
float MathExpressionRandom::getValue() const
{
    double maxval = inputs_[0]->getValue();
    if ( Values::isUdf(maxval) )
	return mUdf(float);

    static int dum mUnusedVar = ensureRandInited();
    return ( float )( maxval * Stats::RandGen::get() );
}


mMathExpressionClass( GaussRandom, 1 )
float MathExpressionGaussRandom::getValue() const
{
    const double stdev = inputs_[0]->getValue();
    if ( Values::isUdf(stdev) )
	return mUdf(float);

    static int dum mUnusedVar = ensureRandInited();
    return ( float ) Stats::RandGen::getNormal(0,stdev);
}


#define mMathExpressionOper( clss, func ) \
class MathExpression##clss : public MathExpression \
{ \
public: \
			MathExpression##clss() \
			    : MathExpression( 1 )		{} \
 \
    float		getValue() const \
			{ \
			    float val = inputs_[0]->getValue(); \
			    if ( Values::isUdf(val) ) \
				return mUdf(float); \
 \
			    float out = func(val); \
			    return out == out ? out : mUdf(float); \
			} \
 \
    MathExpression*	clone() const \
			{ \
			    MathExpression* res = \
					new MathExpression##clss(); \
			    copyInput( res ); \
			    return res; \
			} \
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
			MathExpression##statnm( int nrvals ) \
			    : MathExpression( nrvals )		{} \
 \
    float		getValue() const \
			{ \
			    Stats::RunCalc<float> stats( \
				Stats::CalcSetup().require(Stats::statnm)); \
			    for ( int idx=0; idx<inputs_.size(); idx++) \
				stats += inputs_[idx]->getValue(); \
 \
			    return ( float ) stats.getValue(Stats::statnm); \
			} \
 \
    MathExpression*	clone() const \
			{ \
			    MathExpression* res = \
				new MathExpression##statnm( inputs_.size() ); \
			    copyInput( res ); \
			    return res; \
			} \
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


void MathExpression::setVariableValue( int var, float val )
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
	    const char* str = obj->fullVariableExpression(idx);

	    bool found=false;

	    for ( int idy=0; idy<nrVariables(); idy++ )
	    {
		if ( !strcmp( str, fullVariableExpression(idy) ) )	
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


//--- Parser


BufferString MathExpressionParser::varNameOf( const char* str, int* shift )
{
    if ( shift ) *shift = 0;

    BufferString varnm( str );
    char* ptr = varnm.buf();
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

    if ( vnm.isEqual("this",true) || vnm.isEqual("out",true) )
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


static void countParens( const char* str, int& idx, int& parenslevel, int len )
{
    while ( idx<len && (str[idx]=='(' || parenslevel) )
    {
	if ( str[idx]=='(' ) parenslevel++;
	if ( parenslevel && str[idx]==')' ) parenslevel--;
	if ( parenslevel ) idx++;
    }
}


MathExpression* MathExpressionParser::parse() const
{
    if ( inp_.isEmpty() ) { errmsg_ = "Empty input"; return 0; }
    return parse( inp_.buf() );
}


MathExpression* MathExpressionParser::parse( const char* input ) const
{
    int len = input ? strlen(input) : 0;
    if ( !len ) return 0;

    ArrPtrMan<char> str = new char[len+1];
    str[0]=0;

    int pos = 0;
    for ( int idx=0; idx<len; idx++ )
    {
	if ( !isspace(input[idx]) )
	    str[pos++] = input[idx];
	str[pos] = 0;
    }

    len = strlen(str);

    int parenslevel = 0;
    for ( int idx=0; idx<len; idx++ )
    {
	if ( str[idx]=='(' ) parenslevel++;
	if ( str[idx]==')' ) parenslevel--;
    }
    if ( parenslevel )
    {
	if ( parenslevel > 0 )
	    errmsg_ = "More left than right parentheses";
	else
	    errmsg_ = "More right than left parentheses";
	return 0;
    }

    while ( str[0] == '(' && str[len-1]==')' )
    {
	int localparenslevel = 0;
	bool removeparens = true;

	for ( int idx=1; idx<len-1; idx++ )
	{
	    if ( str[idx]=='(' ) localparenslevel++;
	    if ( str[idx]==')' ) localparenslevel--;

	    if ( localparenslevel<0 )
	    {
		removeparens=false;
		break;
	    }
	}

	if ( !removeparens )
	    break;

	ArrPtrMan<char> tmp = new char[len+1];
	strcpy( tmp, &str[1] );
	tmp[len-2] = 0;
	strcpy( str, tmp );

	len = strlen( str );
    }


    // Look for absolute values
    if ( len>3 &&
	 str[0] == '|' && str[1] != '|' &&
	 str[len-1] == '|' && str[len-2] != '|' )
    {
	ArrPtrMan<char> tmp = new char[len+1];
	strcpy( tmp, &str[1] );
	tmp[len-2] = 0;

	MathExpression* inp = parse( tmp );
	if ( inp )
	{
	    MathExpression* res = new MathExpressionAbs;
	    res->setInput( 0, inp );
	    return res;
	}
    }

    parenslevel = 0;
    bool inabs = false;

    // ? :
    for ( int idx=0; idx<len; idx++ )
    {
	absolute( str, idx, inabs);
	if ( inabs ) continue;

	countParens(str, idx, parenslevel, len);
	if ( parenslevel ) continue;

	if ( str[idx] == '?' )
	{
	    if ( !idx ) continue;

	    ArrPtrMan<char> arg0 = new char[len+1];
	    strcpy( arg0, str );
	    arg0[idx] = 0;

	    for ( int idy=idx; idy<len; idy++ )
	    {
		absolute( str, idx, inabs)
		if ( inabs ) continue;

		countParens(str, idx, parenslevel, len);
		if ( parenslevel ) continue;

		if ( str[idy] == ':' )
		{
		    MathExpression* inp0 = parse( arg0 );
		    if ( !inp0 ) return 0;

		    ArrPtrMan<char> arg1 = new char[len+1];
		    strcpy( arg1, &str[idx+1] );
		    arg1[idy-idx-1] = 0;

		    MathExpression* inp1 = parse( arg1 );
		    if ( !inp1 )
		    {
			delete inp0;
			return 0;
		    }

		    ArrPtrMan<char> arg2 = new char[len+1];
		    strcpy( arg2, &str[idy+1] );

		    MathExpression* inp2 = parse( arg2 );
		    if ( !inp2 )
		    {
			delete inp0;
			delete inp1;
			return 0;
		    }
		
	
		    MathExpression* res = new MathExpressionCondition;

		    res->setInput( 0, inp0 );
		    res->setInput( 1, inp1 );
		    res->setInput( 2, inp2 );

		    return res;
		}
	    }
	}
    }


    // && ||
    for ( int idx=0; idx<len; idx++ )
    {
	absolute( str, idx, inabs)
	if ( inabs ) continue;

	countParens(str, idx, parenslevel, len);
	if ( parenslevel ) continue;

	if ( (str[idx]=='&'&&str[idx+1]=='&')||(str[idx]=='|'&&str[idx+1]=='|'))
	{
	    if ( !idx ) continue;

	    ArrPtrMan<char> arg0 = new char[len+1];
	    strcpy( arg0, str );
	    arg0[idx] = 0;

	    MathExpression* inp0 = parse( arg0 );

	    if ( !inp0 ) return 0;

	    ArrPtrMan<char> arg1 = new char[len+1];
	    strcpy( arg1, &str[idx+2] );

	    MathExpression* inp1 = parse( arg1 );

	    if ( !inp1 )
	    {
		delete inp0;
		return 0;
	    }

	    MathExpression* res = 0;	

	    if ( str[idx] == '&' )
		res = new MathExpressionAND;
	    else 
		res = new MathExpressionOR;

	    res->setInput( 0, inp0 );
	    res->setInput( 1, inp1 );

	    return res;
	}
    }


    // <, >, <=, >=, ==, !=
    for ( int idx=0; idx<len; idx++ )
    {
	absolute( str, idx, inabs)
	if ( inabs ) continue;

	countParens(str, idx, parenslevel, len);
	if ( parenslevel ) continue;

	if ( str[idx]=='<' ||  str[idx]=='>' || str[idx]=='=' || str[idx]=='!')
	{
	    if ( !idx ) continue;
	    if ( (str[idx]=='=' || str[idx]=='!') && str[idx+1] != '=' )
		continue;

	    ArrPtrMan<char> arg0 = new char[len+1];
	    strcpy( arg0, str );
	    arg0[idx] = 0;

	    MathExpression* inp0 = parse( arg0 );

	    if ( !inp0 ) return 0;

	    bool twochars = str[idx+1] == '=' ? true : false;
	    ArrPtrMan<char> arg1 = new char[len+1];
	    strcpy( arg1, &str[idx+1+twochars] );

	    MathExpression* inp1 = parse( arg1 );

	    if ( !inp1 )
	    {
		delete inp0;
		return 0;
	    }

	    MathExpression* res = 0;	

	    if ( str[idx] == '<' )
	    {
		if ( str[idx+1] == '=' )
		    res = new MathExpressionLessOrEqual;
		else
		    res = new MathExpressionLess;
	    }
	    else if ( str[idx] == '>' )
	    {
		if ( str[idx+1] == '=' )
		    res = new MathExpressionMoreOrEqual;
		else
		    res = new MathExpressionMore;
	    }
	    else if ( str[idx] == '=' )
		res = new MathExpressionEqual;
	    else
		res = new MathExpressionNotEqual;

	    res->setInput( 0, inp0 );
	    res->setInput( 1, inp1 );

	    return res;
	}
    }


    // + -
    for ( int idx=0; idx<len; idx++ )
    {
	absolute( str, idx, inabs)
	if ( inabs ) continue;

        countParens(str, idx, parenslevel, len);
	if ( parenslevel ) continue;

	if ( str[idx]=='+' ||  str[idx]=='-' )
	{
	    if ( !idx ) continue;
	    if ( str[idx-1]=='(' ) continue;
	    if ( str[idx-1]=='+' ) continue;
	    if ( str[idx-1]=='-' ) continue;
	    if ( str[idx-1]=='*' ) continue;
	    if ( str[idx-1]=='/' ) continue;
	    if ( str[idx-1]=='^' ) continue;
	    if ( str[idx-1]=='[' ) continue;
	    if ( idx > 1 && caseInsensitiveEqual( &str[idx-1], "e", 1 )
		&& !isalpha(str[idx-2]) ) continue;

	    ArrPtrMan<char> arg0 = new char[len+1];
	    strcpy( arg0, str );
	    arg0[idx] = 0;

	    MathExpression* inp0 = parse( arg0 );

	    if ( !inp0 ) return 0;

	    ArrPtrMan<char> arg1 = new char[len+1];
	    // Always use the plus operator. 
	    // So '20-4-5' becomes '20+-4+-5'
	    if ( str[idx] == '+' )
		strcpy( arg1, &str[idx+1] );
	    else
		strcpy( arg1, &str[idx] );

	    MathExpression* inp1 = parse( arg1 );

	    if ( !inp1 )
	    {
		delete inp0;
		return 0;
	    }

	    MathExpression* res = new MathExpressionPlus;

	    res->setInput( 0, inp0 );
	    res->setInput( 1, inp1 );

	    return res;
	}
    }

#define mParseOperator( op, clss ) \
    for ( int idx=0; idx<len; idx++ ) \
    { \
	absolute( str, idx, inabs) \
	if ( inabs ) continue; \
 \
	countParens(str, idx, parenslevel, len); \
	if ( parenslevel ) continue; \
 \
	if ( str[idx] == op ) \
	{ \
	    if ( !idx ) continue; \
 \
	    ArrPtrMan<char> arg0 = new char[len+1]; \
	    strcpy( arg0, str ); \
	    arg0[idx] = 0; \
	    MathExpression* inp0 = parse( arg0 ); \
	    if ( !inp0 ) return 0; \
 \
	    ArrPtrMan<char> arg1 = new char[len+1]; \
	    strcpy( arg1, &str[idx+1] ); \
	    MathExpression* inp1 = parse( arg1 ); \
	    if ( !inp1 ) \
	    { \
		delete inp0; \
		return 0; \
	    } \
	 \
	    MathExpression* res = new MathExpression##clss; \
 \
	    res->setInput( 0, inp0 ); \
	    res->setInput( 1, inp1 ); \
 \
	    return res; \
	} \
    }

    // negative variables
    if ( str[0]== '-' )
    {
	MathExpression* inp0 = new MathExpressionConstant( -1 );
	if ( !inp0 ) return 0;

	ArrPtrMan<char> arg1 = new char[len+1];
	strcpy( arg1, str+1 );
	MathExpression* inp1 = parse( arg1 );
	if ( !inp1 ) return 0;

	MathExpression* res = new MathExpressionMultiply;
	res->setInput( 0, inp0 );
	res->setInput( 1, inp1 );

	return res;
    }

    mParseOperator( '*', Multiply );
    mParseOperator( '/', Divide );
    mParseOperator( '%', IntDivRest );
    mParseOperator( '|', IntDivide );
    mParseOperator( '^', Power );


    char* endptr;
    double tres = strtod( str, &endptr );

    if ( endptr != str )
	return new MathExpressionConstant( ( float )tres );


#define mParseFunction( func, clss ) { \
    BufferString funcstr(func); funcstr += "("; \
    int funclen = funcstr.size(); \
    if ( !strncasecmp( str, funcstr.buf(), funclen ) && str[len-1] == ')' ) \
    { \
	ArrPtrMan<char> arg0 = new char[len-funclen+1]; \
	strcpy( arg0, str+funclen ); \
	arg0[len-funclen-1] = 0; \
	MathExpression* inp = parse( arg0 ); \
	if ( !inp ) return 0; \
\
	MathExpression* res = new MathExpression##clss; \
	res->setInput( 0, inp ); \
	return res; \
    } \
    }

    // sqrt
    mParseFunction( "sqrt", Sqrt )
    // exp(x) -> e^x
    mParseFunction( "exp", Exp )
    
    // ln (Natural log)  &  log (10log)
    mParseFunction( "ln", NatLog )
    mParseFunction( "log", Log )

//  sin(), asin(), cos(), acos(), tan(), atan()
    mParseFunction( "sin", Sine )
    mParseFunction( "asin", ArcSine )
    mParseFunction( "cos", Cosine )
    mParseFunction( "acos", ArcCosine )
    mParseFunction( "tan", Tangent )
    mParseFunction( "atan", ArcTangent )

    // random number
    mParseFunction( "rand", Random )
    mParseFunction( "randg", GaussRandom )

    if ( (!strncasecmp( str, "min(", 4 ) || 
	  !strncasecmp( str, "max(", 4 ) ||
	  !strncasecmp( str, "sum(", 4 ) ||
	  !strncasecmp( str, "med(", 4 ) ||
	  !strncasecmp( str, "var(", 4 ) ||
	  !strncasecmp( str, "avg(", 4 ) ) && str[len-1] == ')' )
    {
	TypeSet<int> argumentstop;
	for ( int idx=4; idx<len; idx++ )
	{
	    absolute( str, idx, inabs)
	    if ( inabs ) continue;

	    countParens(str, idx, parenslevel, len);
	    if ( parenslevel ) continue;

	    if ( str[idx] == ',' || str[idx] == ')' )
	    {
		if ( !idx ) return 0;

		argumentstop += idx;
		if ( str[idx] == ')' ) break;
	    }
	}

	ObjectSet<MathExpression> inputs_;

	int prevstop = 3;
	for ( int idx=0; idx<argumentstop.size(); idx++ )
	{
	    ArrPtrMan<char> arg = new char[len+1];
	    strncpy( arg, &str[prevstop+1], argumentstop[idx]-prevstop-1);
	    arg[argumentstop[idx]-prevstop-1] = 0;
	    prevstop = argumentstop[idx];
	    
	    MathExpression* inp = parse( arg );
	    if ( !inp )
	    {
		deepErase( inputs_ );
		return 0;
	    }

	    inputs_ += inp;
	}

	MathExpression* res = 0;
	if ( !strncasecmp( str, "max(", 4 ) )
	    res = (MathExpression*) new MathExpressionMax( inputs_.size() );
	else if ( !strncasecmp( str, "min(", 4 ) )
	    res = (MathExpression*) new MathExpressionMin( inputs_.size() );
	else if ( !strncasecmp( str, "sum(", 4 ) )
	    res = (MathExpression*) new MathExpressionSum( inputs_.size() );
	else if ( !strncasecmp( str, "med(", 4 ) )
	    res = (MathExpression*) new MathExpressionMedian( inputs_.size() );
	else if ( !strncasecmp( str, "avg(", 4 ) )
	    res = (MathExpression*) new MathExpressionAverage( inputs_.size() );
	else if ( !strncasecmp( str, "var(", 4 ) )
	    res = (MathExpression*) new MathExpressionVariance( inputs_.size());

	if ( !res )
	    return res;

	for ( int idx=0; idx<inputs_.size(); idx++ )
	    res->setInput( idx, inputs_[idx] );

	return res;
    }
	

    if ( !strcasecmp( input, "pi" ) )
	return new MathExpressionConstant( M_PI );

    if ( !strcasecmp( input, "undef" ) )
	return new MathExpressionConstant( mUdf(float) );

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
	    int recshift;
	    varNameOf( str, &recshift );
	    if ( !recshift )
	    {
		errmsg_ = "Cannot parse this:\n'";
		errmsg_ += input; errmsg_ += "'\n";
		errmsg_ += "Recursive expression must be of type ";
		errmsg_ += "'THIS[-n]' or 'OUT[-n]'";
		return 0;
	    }
	}

	return new MathExpressionVariable( str );
    }

    errmsg_ = "Cannot parse this:\n'";
    errmsg_ += input; errmsg_ += "'";
    return 0;	
}
