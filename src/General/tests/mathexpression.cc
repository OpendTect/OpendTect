/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "mathexpression.h"
#include "testprog.h"
#include "statrand.h"
#include "ptrman.h"


static void parsePassedExpr( const char* inp )
{
    Stats::randGen().init();

    if ( !inp || !*inp ) inp = "| x + |y|z| |";
    od_cout() << "Expression:\n" << inp << "\n\n";
    MathExpressionParser mep( inp );
    MathExpression* me = mep.parse();
    if ( !me )
	od_cout() << "Fail:\n" << mep.errMsg() << od_endl;
    else
    {
	BufferString str; me->dump( str );
	od_cout() << str << od_endl;
	const int nrvars = me->nrVariables();
	od_cout() << od_endl;

	for ( int idx=0; idx<nrvars; idx++ )
	{
	    const float val = (double)(Stats::randGen().getIndex(10) - 5);
	    od_cout() << "Var " << idx << " val=" << val << od_endl;
	    me->setVariableValue( idx, val );
	}

	od_cout() << "\nResult: " << me->getValue() << od_endl;
    }
}

#define mTestExpression( exp, expectation, eps ) \
{ \
    const char* expression = exp; \
    mep.setInput( expression ); \
    PtrMan<MathExpression> me = mep.parse(); \
    \
    mRunStandardTest( me, BufferString("Parsing ", expression ) ); \
    mRunStandardTest( mIsEqual(me->getValue(),expectation, eps), \
	  BufferString( "Value of expression ", expression)); \
}


static bool testArithmeticOrder()
{
    MathExpressionParser mep;

    mTestExpression( "3+4*5", 23, 1e-5 );
    mTestExpression( "4*5+3", 23, 1e-5 );
    mTestExpression( "-3-4*5", -23, 1e-5 );
    mTestExpression( "4*-5-3", -23, 1e-5 );

    mTestExpression( "3+20/5",7, 1e-5 );
    mTestExpression( "20/5+3", 7, 1e-5 );

    mTestExpression( "(3+4)*5", 35, 1e-5 );
    mTestExpression( "4*(5+3)", 32, 1e-5 );
    mTestExpression( "(-3-4)*5", -35, 1e-5 );
    mTestExpression( "4*(-5-3)", -32, 1e-5 );

    mTestExpression( "3+20/5",7, 1e-5 );
    mTestExpression( "20/5+3", 7, 1e-5 );

    mTestExpression( "3+3*2^2",15, 1e-5 );
    mTestExpression( "3+2^2*3", 15, 1e-5 );

    mTestExpression( "1>2 ? 3+2^2*3 : 20/5+3", 7, 1e-5 );
    mTestExpression( "1<2 ? 3+2^2*3 : 20/5+3", 15, 1e-5 );

    mTestExpression( "|1-4|*-2", -6, 1e-5);
    mTestExpression( "|4-1|*-2", -6, 1e-5);

    mTestExpression( "1>=0.17 ? (2<0.6 ? 2:3):1", 3, 1e-5 ); //Mantis bug 2549
    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    if ( clparser.hasKey("expr") )
	parsePassedExpr( argv[2] );
    else
    {
	if ( !testArithmeticOrder() )
	    ExitProgram( 1 );
	//TODO someone extend with more tests
    }

    return ExitProgram( 0 );
}
