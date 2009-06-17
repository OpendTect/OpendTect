/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Mar 2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: mathexpression.cc,v 1.46 2009-06-17 11:56:43 cvsbert Exp $";

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



class MathExpressionVariable : public MathExpression
{
public:
				MathExpressionVariable( const char* str )
				    : MathExpression( 0 )		
				    , str_( new char[strlen(str)+1] )
				{ strcpy( str_, str ); checkVarPrefix(str); }

				~MathExpressionVariable() { delete [] str_; }

    const char*			getVariableStr( int ) const
				{ return str_; }

    int				getNrVariables() const { return 1; }

    float			getValue() const
				{
				    return val_;
				}

    void			setVariable( int, float nv )
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


mMathExpressionClass( Random, 1 )
float MathExpressionRandom::getValue() const
{
    double maxval = inputs_[0]->getValue();
    if ( Values::isUdf(maxval) )
	return mUdf(float);

    Stats::RandGen::init();
    return maxval * Stats::RandGen::get();
}


mMathExpressionClass( GaussRandom, 1 )
float MathExpressionGaussRandom::getValue() const
{
    const double stdev = inputs_[0]->getValue();
    if ( Values::isUdf(stdev) )
	return mUdf(float);

    Stats::RandGen::init( 0 );
    return Stats::RandGen::getNormal(0,stdev);
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
				Stats::RunCalcSetup().require(Stats::statnm)); \
			    for ( int idx=0; idx<inputs_.size(); idx++) \
				stats += inputs_[idx]->getValue(); \
 \
			    return stats.getValue(Stats::statnm); \
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


const char* MathExpression::getVariableStr( int var ) const
{
    if ( var>=getNrVariables() ) return 0;

    int input = (*variableobj_[var])[0];
    int v = (*variablenr_[var])[0];
    return inputs_[input]->getVariableStr( v );
}


void MathExpression::setVariable( int var, float val )
{
    if ( var>=getNrVariables() || var<0 ) return;

    for ( int idx=0; idx<variableobj_[var]->size(); idx++ )
    {
	int input = (*variableobj_[var])[idx];
	int v = (*variablenr_[var])[idx];

	inputs_[input]->setVariable( v, val );
    }
}


bool MathExpression::setInput( int inp, MathExpression* obj )
{
    if ( inp>=0 && inp<getNrInputs() )
    {
	if ( inputs_[inp] ) return false;
	delete inputs_.replace( inp, obj );

	for ( int idx=0; idx<obj->getNrVariables(); idx++ )
	{
	    const char* str = obj->getVariableStr(idx);

	    bool found=false;

	    for ( int idy=0; idy<getNrVariables(); idy++ )
	    {
		if ( !strcmp( str, getVariableStr(idy) ) )	
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
		checkVarPrefix( str );
	    }
	}
		
	return true;
    }

    return true;
}


static void parens( const char* str, int& idx, int& parenslevel, int len )
{
    while ( idx<len && (str[idx]=='(' || parenslevel) )
    {
	if ( str[idx]=='(' ) parenslevel++;
	if ( parenslevel && str[idx]==')' ) parenslevel--;
	if ( parenslevel ) idx++;
    }
}


MathExpression* MathExpression::parse( const char* input )
{
    int len = strlen( input );
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
    if ( parenslevel ) return 0;

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

	parens(str, idx, parenslevel, len);
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

		parens(str, idx, parenslevel, len);
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

	parens(str, idx, parenslevel, len);
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

	parens(str, idx, parenslevel, len);
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

        parens(str, idx, parenslevel, len);
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
	parens(str, idx, parenslevel, len); \
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
    mParseOperator( '^', Power );


    char* endptr;
    double tres = strtod( str, &endptr );

    if ( endptr != str )
	return new MathExpressionConstant( tres );


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

	    parens(str, idx, parenslevel, len);
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
	return new MathExpressionVariable( str );

    return 0;	
}


MathExpression::VarType MathExpression::getType( int ivar ) const
{
    BufferString pfx; int shft;
    getPrefixAndShift( getVariableStr(ivar), pfx, shft );

    if ( pfx == "THIS" )
	return Recursive;
    else if ( pfx[0] == 'c' || pfx[0] == 'C' )
	return Constant;

    return Variable;
}


int MathExpression::getUsrVarIdx( int ivar ) const
{
    if ( getType(ivar) == Recursive )
	return -1;

    BufferString pfx; int shft;
    getPrefixAndShift( getVariableStr(ivar), pfx, shft );

    const char* ptr = pfx.buf();
    while ( *ptr && !isdigit(*ptr) )
	ptr++;

    return atoi( ptr );
}


void MathExpression::getPrefixAndShift( const char* str,
					BufferString& varprefix, int& shift )
{
    int startbracketidx = -1;
    int endbracketidx = -1;
    for ( int charidx=0; charidx<strlen(str); charidx++ )
    {
	if ( str[charidx]=='[' )
	    startbracketidx = charidx;
	if ( str[charidx]==']' )
	{
	    endbracketidx = charidx;
	    break;
	}
    }

    int nrchars = startbracketidx>-1 ? startbracketidx : strlen(str);
    ArrPtrMan<char> prefix = new char [nrchars+1];
    strncpy( prefix, &str[0], nrchars );
    prefix[nrchars] = 0;

    if ( endbracketidx == -1 && startbracketidx > -1 )
	shift = mUdf(int);
    else
    {
	nrchars = endbracketidx>-1 ? endbracketidx - startbracketidx - 1 : 0;
	if ( !nrchars )
	    shift = 0;
	else
	{
	    ArrPtrMan<char> shiftstr = new char [nrchars+1];
	    strncpy( shiftstr, &str[startbracketidx+1], nrchars );
	    shiftstr[nrchars] = 0;
	    shift = isNumberString(shiftstr,mC_False)
		  ? atoi( shiftstr ) : mUdf(int);
	}
    }
    
    varprefix = prefix.ptr();
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


int MathExpression::getNrVariables() const
{ return variableobj_.size(); }


int MathExpression::getNrDiffVariables() const
{
    return varprefixes_.size();
}


void MathExpression::copyInput( MathExpression* target ) const
{
    int sz = getNrInputs();

    for ( int idx=0; idx<sz; idx++ )
	target->setInput(idx, inputs_[idx]->clone() );
}


void MathExpression::checkVarPrefix( const char* str )
{
    BufferString prefix;
    int shift;
    getPrefixAndShift( str, prefix, shift );

    if ( !strcmp( prefix, "THIS" ) )
    {
	isrecursive_ = true;
	return;
    }

    varprefixes_.addIfNew( prefix );
}


int MathExpression::getPrefixIdx( const char* str, bool skipcstvar ) const
{
    if ( skipcstvar )
    {
	int nrcstpreffound = 0;
	for ( int idx=0; idx<varprefixes_.size(); idx++ )
	{
	    if ( !strcmp( str, getVarPrefixStr(idx) ) )
		return idx - nrcstpreffound;
	    else if ( !strncmp( getVarPrefixStr(idx), "c", 1 ) )
		nrcstpreffound++;
	}
    }
    
    return varprefixes_.indexOf(str);
}
