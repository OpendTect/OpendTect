/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "mathformula.h"
#include "testprog.h"
#include "unitofmeasure.h"
#include "iopar.h"


#define mTestValErr(var,val) \
	{ od_cout() << "Fail:\n" << #var <<'='<< var << \
	    ", not " << val << od_endl; return false; }
#define mTestValSucces(var,val) \
    if ( !quiet ) od_cout() << "Success: " << #var <<'='<< var << od_endl

#define mTestVal(var,val) \
    if ( var != val ) mTestValErr(var,val) mTestValSucces(var,val)
#define mTestValF(var,val) \
    if ( !mIsEqual(var,val,0.001) ) mTestValErr(var,val) mTestValSucces(var,val)


static bool testSimpleFormula()
{
     const char* expr = "c0 * x + y - this[-2]";

    if ( !quiet )
	od_cout() << "Expression: '" << expr << "'\n";

    Math::Formula form( expr );

    if ( !form.isOK() )
	{ od_cout() << "Fail:\n" << form.errMsg() << od_endl; return false; }

    const int nrinp = form.nrInputs();
    mTestVal(nrinp,3);

    const int nrshft = form.maxRecShift();
    mTestVal(nrshft,2);

    form.recStartVals()[0] = 3;
    form.recStartVals()[1] = 4;

    form.setInputUnit( 1, UoMR().get("ft") );

    float inpvals[3];
    inpvals[0] = 1; inpvals[1] = 2; inpvals[2] = 3;
    float val = form.getValue( inpvals, true );
    mTestValF(val,6.56168);

    val = form.getValue( inpvals, true );
    mTestValF(val,5.56168);

    form.startNewSeries();
    form.setOutputUnit( UoMR().get("ms") );
    val = form.getValue( inpvals, false );
    mTestValF(val,6.56168);
    val = form.getValue( inpvals, true );
    mTestValF(val,0.00556168);

    if ( !quiet )
    {
	IOPar iop;
	form.fillPar( iop );
	BufferString str;
	iop.dumpPretty( str );
	od_cout() << str << od_endl;
	Math::Formula form2( "" );
	form2.usePar( iop );
	IOPar iop2;
	form2.fillPar( iop2 );
	str.setEmpty();
	iop2.dumpPretty( str );
	od_cout() << str << od_endl;
    }

    return true;
}


static bool testRepeatingVar()
{
     const char* expr = "x + x + y";

    if ( !quiet )
	od_cout() << "Expression: '" << expr << "'\n";

    Math::Formula form( expr );

    if ( !form.isOK() )
	{ od_cout() << "Fail:\n" << form.errMsg() << od_endl; return false; }

    const int nrinp = form.nrInputs();
    mTestVal(nrinp,2);

    const int nrshft = form.maxRecShift();
    mTestVal(nrshft,0);

    float inpvals[2];
    inpvals[0] = 3; inpvals[1] = 7;
    float val = form.getValue( inpvals, true );
    mTestValF(val,13);

    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testSimpleFormula() )
	ExitProgram( 1 );

    if ( !testRepeatingVar() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
