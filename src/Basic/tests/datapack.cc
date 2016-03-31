/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2014
 * FUNCTION :
-*/


#include "datapack.h"
#include "ptrman.h"

#include "testprog.h"

class DataPackClass : public BufferDataPack
{
public:
		DataPackClass(bool& deletedflag)
		    : deletedflag_( deletedflag ) { deletedflag_ = false; }
		~DataPackClass() { deletedflag_ = true; }

    bool	hasManager() const { return manager_; }

    bool&	deletedflag_;
};


bool testDataPack()
{
    bool deleted = false;

    DataPackClass* dpc = new DataPackClass( deleted );
    mRunStandardTest( dpc->id()!=DataPack::cNoID(), "Id assignment");

    delete dpc;
    mRunStandardTest( deleted, "Normal delete" );

    dpc = new DataPackClass( deleted );

    DataPackMgr& dpm = DPM(DataPackMgr::BufID());
    dpm.addAndObtain( dpc );

    mRunStandardTest( dpc->hasManager(), "Manager set");

    mRunStandardTest( dpc->nrRefs()==1, "Nr users after addAndObtain");

    int id = dpc->id();

    DataPack* dp = dpm.observe( id );
    mRunStandardTest( dpc->nrRefs()==1, "Nr users after observation obtain");

    mRunStandardTest( dp, "Return value from observation obtain");

    {
	//Note, this is not a standard way to create the ref, but I need to
	//send them out of scope by will
	PtrMan<ConstDataPackRef<BufferString> > faultyref =
	    new DataPackRef<BufferString>( dpm.obtain(id) );

	mRunStandardTest( dpc->nrRefs()==2, "Nr users after second obtain");

	mRunStandardTest( !faultyref->ptr(), "Refusal to give invalid cast");

	//Note, this is not a standard way to create the ref, but I need to
	//send them out of scope by will
	PtrMan<DataPackRef<DataPackClass> > goodref =
		    new DataPackRef<DataPackClass>( dpm.obtain(id) );

	mRunStandardTest( dpc->nrRefs()==3, "Nr users after third obtain");

	faultyref = 0;
	mRunStandardTest( dpc->nrRefs()==2,
			  "Nr users after ref goes out of scope");

	//Note, this is not a standard way to create the ref, but I need to
	//send them out of scope by will
	PtrMan<DataPackRef<DataPackClass> > goodref2 =
	    new DataPackRef<DataPackClass>( *goodref );

	mRunStandardTest( dpc->nrRefs()==3,
			 "Nr users after ref copy constructor.");

	goodref = 0;
	goodref2 = 0;

	dp->unRef();
	mRunStandardTest( deleted, "Release delete" );

	dp = dpm.obtain( id );
	mRunStandardTest( !dp, "Refusal of returning released");

    }
    {
	DataPackClass* dpa = new DataPackClass( deleted );
	dpm.addAndObtain( dpa );
	DataPackClass* dpb = new DataPackClass( deleted );
	dpm.addAndObtain( dpb );

	DataPackRef<DataPackClass> dparef = dpa;
	DataPackRef<DataPackClass> dpbref = dpb;

	DataPackRef<DataPackClass> testref = 0;
	testref = dparef;

	mRunStandardTest( dpa->nrRefs()==2,
			 "Nr users after assignment operator");
	testref = dpbref;
	mRunStandardTest( dpa->nrRefs()==1,
			 "Nr users after second assignement operator");
    }

    return true;
}



int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testDataPack() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
