/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "mathexpression.h"
#include "testprog.h"
#include "statrand.h"
#include "ptrman.h"


static void parsePassedExpr( const char* inp )
{
    if ( !inp || !*inp ) inp = "abs(x + idiv(y,z))";
    od_cout() << "Expression:\n" << inp << "\n\n";
    Math::ExpressionParser mep( inp );
    Math::Expression* me = mep.parse();
    if ( !me )
	od_cout() << "Fail:\n" << mep.errMsg() << od_endl;
    else
    {
	BufferString str; me->dump( str );
	od_cout() << str << od_endl;
	const int nrvars = me->nrVariables();
	od_cout() << od_endl;

	Stats::RandGen gen;
	for ( int idx=0; idx<nrvars; idx++ )
	{
	    const float val = (float)gen.getInt(-5,15);
	    od_cout() << "Var " << idx << " val=" << val << od_endl;
	    me->setVariableValue( idx, val );
	}

	od_cout() << "\nResult: " << me->getValue() << od_endl;
    }

    if ( mep.foundOldAbs() )
	od_cout() << "\n[Old abs value used]" << od_endl;
}

#define mTestExpression( exp, expectation, eps ) \
{ \
    const char* expression = exp; \
    mep.setInput( expression ); \
    PtrMan<Math::Expression> me = mep.parse(); \
    \
    mRunStandardTest( me, BufferString("Parsing ", expression ) ); \
    mRunStandardTest( mIsEqual(me->getValue(),expectation, eps), \
	  BufferString( "Value of expression ", expression)); \
}


static bool testArithmeticOrder()
{
    Math::ExpressionParser mep;

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

    mTestExpression( "0.17<1 ? 1:(2<0.6 ? 2:3)", 1, 1e-5 );
    mTestExpression( "1>=0.17 ? (2<0.6 ? 2:3):1", 3, 1e-5 ); //Mantis bug 2549

    mTestExpression( "(10>5 && 5<0) ? 1:0", 0, 1e-5 );
    mTestExpression( "(10>5 || 5<0) ? 1:0", 1, 1e-5 );

    mTestExpression( "4==4 ? 1 : 0", 1, 1e-5 );
    mTestExpression( "3!=4 ? 1 : 0", 1, 1e-5 );
    mTestExpression( "9%5", 4, 1e-5 );
    return true;
}


static bool testParenthesis()
{
    Math::ExpressionParser mep;

    mTestExpression( "16+(8/4)", 18, 1e-5 );
    mTestExpression( "(16+8)/4", 6, 1e-5 );
    mTestExpression( "(16+8)/(4/2)", 12, 1e-5 );
    mTestExpression( "(16+8)/((4/2)/2)", 24, 1e-5 );
    mTestExpression( "16+(8/((4/2)/8))", 48, 1e-5 );
    mTestExpression( "(9%7)%(7%4)", 2, 1e-5 );
    mTestExpression( "(9%7)*(7%4)", 6, 1e-5 );
    mTestExpression( "(9-8) ? ((3-3) ? 4 : 5) : 0", 5, 1e-5 );
    mTestExpression( "(9-9) ? 0 : ((3-3) ? 4 : 5)", 5, 1e-5 );
    mTestExpression( "(0 ? 2:3) ? (4 ? 5:6) : (5 ? 6:7)", 5, 1e-5 );
    return true;
}


static bool testAbs()
{
    Math::ExpressionParser mep;

    mTestExpression( "abs(-8)+abs(-4)", 12, 1e-5 );
    mTestExpression( "abs(-8)-abs(-4)", 4, 1e-5 );
    mTestExpression( "abs(-8)*abs(-4)", 32, 1e-5 );
    mTestExpression( "abs(-8)/abs(-4)", 2, 1e-5 );
    mTestExpression( "abs(-9)%abs(-4)", 1, 1e-5 );
    mTestExpression( "abs(-3)/abs(-2)", 1.5, 1e-5 );
    mTestExpression( "abs(0 ? -2:-3)>0 ? abs(-4 ? -5:-6) : abs(-5 ? -6:-7)",
					5, 1e-5 );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( clParser().hasKey("expr") )
	parsePassedExpr( argv[2] );
    else
    {
	if ( !testArithmeticOrder() )
	    return 1;
	if ( !testParenthesis() )
	    return 1;
	if ( !testAbs() )
	    return 1;
    }

    return 0;
}
