/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID = "$Id";

#include "mathexpressionimpl.h"
#include "ctype.h"
#include "ptrman.h"

#define absolute( str, idx, inabs) \
    if ( str[idx]=='|' && !(str[idx+1]=='|' || (idx && str[idx-1]=='|') ) ) \
	inabs=(inabs+1)%2;

#ifndef M_PI
# define M_PI           3.14159265358979323846  /* pi */
#endif


const char* MathExpression::getVariableStr( int var ) const
{
    if ( var>=getNrVariables() ) return 0;

    int input = (*variableobj[var])[0];
    int v = (*variablenr[var])[0];
    return inputs[input]->getVariableStr( v );
}


void MathExpression::setVariable( int var, float val )
{
    if ( var>=getNrVariables() ) return;

    for ( int idx=0; idx<variableobj[var]->size(); idx++ )
    {
	int input = (*variableobj[var])[idx];
	int v = (*variablenr[var])[idx];

	inputs[input]->setVariable( v, val );
    }
}


bool MathExpression::setInput( int inp, MathExpression* obj )
{
    if ( inp < getNrInputs() )
    {
	if ( inputs[inp] ) return false;
	inputs.replace( obj, inp );

	for ( int idx=0; idx<obj->getNrVariables(); idx++ )
	{
	    const char* str = obj->getVariableStr(idx);

	    bool found=false;

	    for ( int idy=0; idy<getNrVariables(); idy++ )
	    {
		if ( !strcmp( str, getVariableStr(idy) ) )	
		{
		    (*variableobj[idy]) += inp;
		    (*variablenr[idy]) += idx;
		    found = true;
		    break;
		}
	    }

	    if ( !found )
	    {
		variableobj += new TypeSet<int>( 1, inp );
		variablenr += new TypeSet<int>( 1, idx );
	    }
	}
		
	return true;
    }

    return true;
}

static void parens( const char* str, int& idx, int& parenslevel, int len)
{
    do
    {
	if ( str[idx]=='(' ) parenslevel++;
	if ( str[idx]==')' ) parenslevel--;
	if ( parenslevel ) idx++;
    } while ( parenslevel && idx<len );
}


MathExpression* MathExpression::parse( const char* input )
{
    int len = strlen( input );

    ArrPtrMan<char> str = new char[len+1];

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
	int parenslevel = 0;
	bool removeparens = true;

	for ( int idx=1; idx<len-1; idx++ )
	{
	    if ( str[idx]=='(' ) parenslevel++;
	    if ( str[idx]==')' ) parenslevel--;

	    if ( parenslevel<0 )
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
	if ( !inp )
	{
	    return 0;
	}

	MathExpression* res = new MathExpressionAbs;
	res->setInput( 0, inp );

	return res;
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
	    {
		res = new MathExpressionAND;
	    }
	    else 
		res = new MathExpressionOR;

	    res->setInput( 0, inp0 );
	    res->setInput( 1, inp1 );

	    return res;
	}
    }


    // <, >, <=, >=, ==
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
	    if ( idx > 1 && caseInsensitiveEqual( &str[idx-1], "e", 1 )
		&& !isalpha(str[idx-2]) ) continue;

	    ArrPtrMan<char> arg0 = new char[len+1];
	    strcpy( arg0, str );
	    arg0[idx] = 0;

	    MathExpression* inp0 = parse( arg0 );

	    if ( !inp0 ) return 0;

	    ArrPtrMan<char> arg1 = new char[len+1];
	    strcpy( arg1, &str[idx+1] );

	    MathExpression* inp1 = parse( arg1 );

	    if ( !inp1 )
	    {
		delete inp0;
		return 0;
	    }

	    MathExpression* res = str[idx]  == '+' 
				? (MathExpression*) new MathExpressionPlus
				: (MathExpression*) new MathExpressionMinus;

	    res->setInput( 0, inp0 );
	    res->setInput( 1, inp1 );

	    return res;
	}
    }


    // * /
    for ( int idx=0; idx<len; idx++ )
    {
	absolute( str, idx, inabs)
	if ( inabs ) continue;

	parens(str, idx, parenslevel, len);
	if ( parenslevel ) continue;

	if ( str[idx] == '*' ||  str[idx] == '/' )
	{
	    if ( !idx ) continue;

	    ArrPtrMan<char> arg0 = new char[len+1];
	    strcpy( arg0, str );
	    arg0[idx] = 0;

	    MathExpression* inp0 = parse( arg0 );

	    if ( !inp0 ) return 0;

	    ArrPtrMan<char> arg1 = new char[len+1];
	    strcpy( arg1, &str[idx+1] );

	    MathExpression* inp1 = parse( arg1 );

	    if ( !inp1 )
	    {
		delete inp0;
		return 0;
	    }
	
	    MathExpression* res;

	    if ( str[idx]=='*' ) res = new MathExpressionMultiply;
	    if ( str[idx]=='/' ) res = new MathExpressionDivide;

	    res->setInput( 0, inp0 );
	    res->setInput( 1, inp1 );

	    return res;
	}
    }

    // Power of (^)

    for ( int idx=0; idx<len; idx++ )
    {
	absolute( str, idx, inabs)
	if ( inabs ) continue;

	parens(str, idx, parenslevel, len);
	if ( parenslevel ) continue;

	if ( str[idx] == '^' )
	{
	    if ( !idx ) continue;

	    ArrPtrMan<char> arg0 = new char[len+1];
	    strcpy( arg0, str );
	    arg0[idx] = 0;

	    MathExpression* inp0 = parse( arg0 );

	    if ( !inp0 ) return 0;

	    ArrPtrMan<char> arg1 = new char[len+1];
	    strcpy( arg1, &str[idx+1] );

	    MathExpression* inp1 = parse( arg1 );

	    if ( !inp1 )
	    {
		delete inp0;
		return 0;
	    }
	
	    MathExpression* res = new MathExpressionPower;

	    res->setInput( 0, inp0 );
	    res->setInput( 1, inp1 );

	    return res;
	}
    }

    char* endptr;
    double tres = strtod( str, &endptr );

    if ( endptr != str )
	return new MathExpressionConstant( tres );


    if ( !strncmp( str, "sqrt(", 5 ) && str[len-1] == ')' )
    {
	ArrPtrMan<char> arg0 = new char[len-5];
	strcpy( arg0, str+5 );
	arg0[len-6] = 0;

	MathExpression* inp0 = parse( arg0 );
	if ( !inp0 ) return 0;

	MathExpression* inp1 = parse( "0.5" );

	if ( !inp1 )
	{
	    delete inp0;
	    return 0;
	}
    
	MathExpression* res = new MathExpressionPower;

	res->setInput( 0, inp0 );
	res->setInput( 1, inp1 );

	return res;
	
    }


    // exp(x) -> e^x
    if ( !strncmp( str, "exp(", 4 ) && str[len-1] == ')' )
    {
	MathExpression* inp0 = parse( "2.7182818284590452354" );

	if ( !inp0 ) return 0;

	ArrPtrMan<char> arg1 = new char[len+1];
	strcpy( arg1, str+4 );
	arg1[len-5] = 0;

	MathExpression* inp1 = parse( arg1 );

	if ( !inp1 )
	{
	    delete inp0;
	    return 0;
	}
    
	MathExpression* res = new MathExpressionPower;

	res->setInput( 0, inp0 );
	res->setInput( 1, inp1 );

	return res;
	
    }


    // ln (Natural log)  &  log (10log)
    if ( !strncmp( str, "ln(", 3 ) && str[len-1] == ')' )
    {
	ArrPtrMan<char> arg0 = new char[len-3];
	strcpy( arg0, str+3 );
	arg0[len-4] = 0;

	MathExpression* inp = parse( arg0 );

	if ( !inp ) return false;

	MathExpression* res = new MathExpressionNatLog;
	res->setInput( 0, inp );

	return res;
    }


    if ( !strncmp( str, "log(", 4 ) && str[len-1] == ')' )
    {
	ArrPtrMan<char> arg0 = new char[len-4];
	strcpy( arg0, str+4 );
	arg0[len-5] = 0;

	MathExpression* inp = parse( arg0 );

	if ( !inp ) return false;

	MathExpression* res = new MathExpressionLog;
	res->setInput( 0, inp );

	return res;
    }


//  sin(), cos(), tan()
    if ( !strncmp( str, "sin(", 4 ) && str[len-1] == ')' )
    {
	ArrPtrMan<char> arg0 = new char[len-4];
	strcpy( arg0, str+4 );
	arg0[len-5] = 0;

	MathExpression* inp = parse( arg0 );

	if ( !inp ) return false;

	MathExpression* res = new MathExpressionSine;
	res->setInput( 0, inp );

	return res;
    }


    if ( !strncmp( str, "cos(", 4 ) && str[len-1] == ')' )
    {
	ArrPtrMan<char> arg0 = new char[len-4];
	strcpy( arg0, str+4 );
	arg0[len-5] = 0;

	MathExpression* inp = parse( arg0 );

	if ( !inp ) return false;

	MathExpression* res = new MathExpressionCosine;
	res->setInput( 0, inp );

	return res;
    }


    if ( !strncmp( str, "tan(", 4 ) && str[len-1] == ')' )
    {
	ArrPtrMan<char> arg0 = new char[len-4];
	strcpy( arg0, str+4 );
	arg0[len-5] = 0;

	MathExpression* inp = parse( arg0 );

	if ( !inp ) return false;

	MathExpression* res = new MathExpressionTangent;
	res->setInput( 0, inp );

	return res;
    }


    // negative variables
    if ( str[0]== '-' )
    {
	MathExpression* inp0 = parse( "-1" );
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


    if ( !strcmp( input, "pi" ) )
	return new MathExpressionConstant( M_PI );
	

    bool isvariable = true;

    for ( int idx=0; idx<len; idx++ )
    {
	if ( (!idx&&isdigit(str[idx])) || !isalnum(str[idx]))
	{
	    isvariable = false;
	    break;
	}
    }

    if ( isvariable )
    {
	return new MathExpressionVariable( str );
    }

	


    return 0;	
}


MathExpression::MathExpression( int sz )
{
    inputs.allowNull();
    for ( int idx=0; idx<sz; idx++ )
	inputs += 0;
}


MathExpression::~MathExpression( )
{
    deepErase( inputs );
    deepErase( variableobj );
    deepErase( variablenr );
}


int MathExpression::getNrVariables() const
{ return variableobj.size(); }


void MathExpression::copyInput( MathExpression* target ) const
{
    int sz = getNrInputs();

    for ( int idx=0; idx<sz; idx++ )
	target->setInput(idx, inputs[idx]->clone() );
}
	
	    
