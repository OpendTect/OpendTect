/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID = "$Id";

#include <mathexpressionimpl.h>
#include <ctype.h>


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



MathExpression* MathExpression::parse( const char* input )
{
    int len = strlen( input );
    char str[len];

    int pos = 0;
    for ( int idx=0; idx<len; idx++ )
    {
	if ( !isspace(input[idx]) )
	    str[pos++] = input[idx];
	str[pos] = 0;
    }

    len = strlen(str);

    while ( str[0] == '(' && str[len-1]==')' )
    {
	int bracketlevel = 0;
	bool removebracket = true;

	for ( int idx=1; idx<len-1; idx++ )
	{
	    if ( str[idx]=='(' ) bracketlevel++;
	    if ( str[idx]==')' ) bracketlevel--;

	    if ( bracketlevel<0 )
	    {
		removebracket=false;
		break;
	    }
	}

	if ( !removebracket )
	    break;

	char tmp[len];
	strcpy( tmp, &str[1] );
	tmp[len-2] = 0;
	strcpy( str, tmp );

	len = strlen( str );
    }
	

    int bracketlevel = 0;

    for ( int idx=0; idx<len; idx++ )
    {
	if ( str[idx]=='(' ) bracketlevel++;
	if ( str[idx]==')' ) bracketlevel--;

	if ( bracketlevel ) continue;

	if ( str[idx]=='+' ||  str[idx]=='-' )
	{
	    if ( !idx ) continue;
	    if ( str[idx-1]=='(' ) continue;
	    if ( str[idx-1]=='+' ) continue;
	    if ( str[idx-1]=='-' ) continue;
	    if ( str[idx-1]=='*' ) continue;
	    if ( str[idx-1]=='/' ) continue;
	    if ( str[idx-1]=='^' ) continue;

	    char arg0[len];
	    strcpy( arg0, str );
	    arg0[idx] = 0;

	    MathExpression* inp0 = parse( arg0 );

	    if ( !inp0 ) return 0;

	    char arg1[len];
	    strcpy( arg1, &str[idx+1] );

	    MathExpression* inp1 = parse( arg1 );

	    if ( !inp1 )
	    {
		delete inp0;
		return 0;
	    }

	    MathExpression* res = str[idx] == '+'
				? new MathExpressionPlus
				: new MathExpressionMinus;

	    res->setInput( 0, inp0 );
	    res->setInput( 1, inp1 );

	    return res;
	}
    }

    for ( int idx=0; idx<len; idx++ )
    {
	if ( str[idx]=='(' ) bracketlevel++;
	if ( str[idx]==')' ) bracketlevel--;

	if ( bracketlevel ) continue;

	if ( str[idx] == '*' ||  str[idx] == '/' )
	{
	    if ( !idx ) continue;

	    char arg0[len];
	    strcpy( arg0, str );
	    arg0[idx] = 0;

	    MathExpression* inp0 = parse( arg0 );

	    if ( !inp0 ) return 0;

	    char arg1[len];
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


    for ( int idx=0; idx<len; idx++ )
    {
	if ( str[idx]=='(' ) bracketlevel++;
	if ( str[idx]==')' ) bracketlevel--;

	if ( bracketlevel ) continue;

	if ( str[idx] == '^' )
	{
	    if ( !idx ) continue;

	    char arg0[len];
	    strcpy( arg0, str );
	    arg0[idx] = 0;

	    MathExpression* inp0 = parse( arg0 );

	    if ( !inp0 ) return 0;

	    char arg1[len];
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

    bool isvariable = true;

    for ( int idx=0; idx<len; idx++ )
    {
	if ( (!idx&&isdigit(str[idx])) || !isalpha(str[idx]))
	{
	    isvariable = false;
	    break;
	}
    }

    if ( isvariable )
    {
	return new MathExpressionVariable( str );
    }

	

    if ( !strcmp( input, "pi" ) )
	return new MathExpressionConstant( M_PI );
	

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
	
	    
