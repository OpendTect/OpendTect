/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "mathformula.h"
#include "mathspecvars.h"
#include "testprog.h"
#include "unitofmeasure.h"
#include "iopar.h"


#define mTestValErr(var,val) \
	{ od_cout() << "Fail:\n" << #var <<'='<< var << \
	    ", not " << val << od_endl; return false; }
#define mTestValSucces(var,val) \
    logStream() << "Success: " << #var <<'='<< var << od_endl

#define mTestVal(var,val) \
    if ( var != val ) mTestValErr(var,val) mTestValSucces(var,val)
#define mTestValF(var,val) \
    if ( !mIsEqual(var,val,0.001) ) mTestValErr(var,val) mTestValSucces(var,val)


static bool testSimpleFormula()
{
     const char* expr = "c0 * x + y - this[-2]";

    if ( !quiet_ )
	od_cout() << "Expression: '" << expr << "'\n";

    Math::Formula tryform( false, expr );
    if ( tryform.isOK() )
	{ od_cout() << "Fail:\n" << expr
	    << " should not parse in single mode" << od_endl; return false; }
    if ( !quiet_ )
	od_cout() << "OK, single mode err msg='" << tryform.errMsg() << "'\n";

    Math::Formula form( true, expr );

    if ( !form.isOK() )
	{ od_cout() << "Fail:\ndata series mode errmsg="
			<< form.errMsg() << od_endl; return false; }

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

    if ( !quiet_ )
    {
	IOPar iop;
	form.fillPar( iop );
	BufferString str;
	iop.dumpPretty( str );
	od_cout() << str << od_endl;
	Math::Formula form2( true, "" );
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
     const char* expr = "x[-1] + 2*y + out[-1] + x[1] + aap";

    if ( !quiet_ )
	od_cout() << "Expression: '" << expr << "'\n";

    Math::SpecVarSet svs;
    svs += Math::SpecVar( "Aap", "Dit is aapje", true, Mnemonic::Dist );
    svs += Math::SpecVar( "Noot", "Dit is nootje", false );
    Math::Formula form( true, svs, expr );

    if ( !form.isOK() )
	{ od_cout() << "Fail:\n" << form.errMsg() << od_endl; return false; }

    const int nrinp = form.nrInputs();
    mTestVal(nrinp,3);

    const int nrshft = form.maxRecShift();
    mTestVal(nrshft,1);

    const int nrinpvals = form.nrValues2Provide();
    mTestVal(nrinpvals,4);
    mTestVal(form.isSpec(0),false);
    mTestVal(form.isSpec(2),true);

    float inpvals[4];
    inpvals[0] = -3; inpvals[1] = 7; // values for x[-1] and x[1]
    inpvals[2] = 11; // value for y
    inpvals[3] = -10; // value for aap
    float val = form.getValue( inpvals, true );
    mTestValF(val,16);

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testSimpleFormula() )
	return 1;

    if ( !testRepeatingVar() )
	return 1;

    return 0;
}
