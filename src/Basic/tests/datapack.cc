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


    bool	hasManager() const { return manager_; }

    bool&	deletedflag_;
protected:
		~DataPackClass() { deletedflag_ = true; }
};

class DataPackClass2 : public BufferDataPack
{
public:
		DataPackClass2(bool& deletedflag)
		    : deletedflag_( deletedflag ) { deletedflag_ = false; }


    bool	hasManager() const { return manager_; }

    bool&	deletedflag_;
protected:
		~DataPackClass2() { deletedflag_ = true; }
};


bool testDataPack()
{
    bool deleted = false;
    {
	DataPackClass* dpc = new DataPackClass( deleted );
	mRunStandardTest( dpc->id()!=DataPack::cNoID(), "Id assignment" );

	dpc->ref();
	dpc->unRef();

	mRunStandardTest( deleted, "Normal delete" );
    }
    {
	RefMan<DataPackClass> dpc = new DataPackClass( deleted );

	DataPackMgr& dpm = DPM(DataPackMgr::BufID());
	dpm.add( dpc );

	mRunStandardTest( !deleted && dpc->hasManager(), "Manager set" );
	mRunStandardTest( !deleted && dpc->nrRefs()==1, "Nr users after add" );

	DataPackID id = dpc->id();

	WeakPtr<DataPack> wptr = dpm.observeDP( id );
	mRunStandardTest( !deleted && dpc->nrRefs()==1,
			  "Nr users after observe" );

	WeakPtr<DataPackClass2> dpc2obs = dpm.observe<DataPackClass2>( id );
	mRunStandardTest( !deleted && dpc->nrRefs()==1,
			 "Nr users after observe of wrong class" );

	WeakPtr<DataPackClass> dpcobs = dpm.observe<DataPackClass>( id );
	mRunStandardTest( !deleted && dpc->nrRefs()==1,
			 "Nr users after observe of correct class" );

	auto dp = dpm.getDP( id );
	mRunStandardTest( !deleted && dpc->nrRefs()==2, "Nr users after get" );

	RefMan<DataPackClass> dpcman = dpm.get<DataPackClass>( id );
	RefMan<DataPackClass2> fdp = dpm.get<DataPackClass2>( id );
	mRunStandardTest( !deleted && dpc->nrRefs()==3, "Nr users after get" );

	dpc = nullptr;
	dp = nullptr;
	dpcman = nullptr;
	mRunStandardTest( deleted && !wptr, "Weak pointer set to zero" );
    }
    return true;
}



int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testDataPack() )
	return 1;

    return 0;
}
