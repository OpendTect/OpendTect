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

    DataPackClass* dpc = new DataPackClass( deleted );
    mRunStandardTest( dpc->id()!=DataPack::cNoID(), "Id assignment");

    dpc->ref();
    dpc->unRef();
    
    mRunStandardTest( deleted, "Normal delete" );

    dpc = new DataPackClass( deleted );

    DataPackMgr& dpm = DPM(DataPackMgr::BufID());
    dpm.add( dpc );

    mRunStandardTest( !deleted && dpc->hasManager(), "Manager set");
    mRunStandardTest( !deleted && dpc->nrRefs()==0, "Nr users after add");

    int id = dpc->id();

    WeakPtr<DataPack> wptr = dpm.observe( id );
    mRunStandardTest( !deleted && dpc->nrRefs()==0,
                     "Nr users after observe");
    
    WeakPtr<DataPackClass2> dpc2obs = dpm.observeAndCast<DataPackClass2>( id );
    mRunStandardTest( !deleted && dpc->nrRefs()==0,
                     "Nr users after observeAndCast of wrong class");
    
    
    WeakPtr<DataPackClass> dpcobs = dpm.observeAndCast<DataPackClass>( id );
    mRunStandardTest( !deleted && dpc->nrRefs()==0,
                     "Nr users after observeAndCast of correct class");


    RefMan<DataPack> dp = dpm.get( id );
    
    mRunStandardTest( !deleted && dpc->nrRefs()==1,
                      "Nr users after get");
    
    RefMan<DataPackClass> dpcman = dpm.getAndCast<DataPackClass>(id);
    RefMan<DataPackClass2> fdp = dpm.getAndCast<DataPackClass2>(id);
    
    mRunStandardTest( !deleted && dpc->nrRefs()==2,
                      "Nr users after getAndCast");
    
    
    dp = 0;
    dpcman = 0;
    
    mRunStandardTest( deleted && !wptr,
                     "Weak pointer set to zero");

    return true;
}



int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testDataPack() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
