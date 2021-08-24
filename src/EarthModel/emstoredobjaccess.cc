/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/


#include "emstoredobjaccess.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfaceiodata.h"
#include "executor.h"
#include "threadwork.h"
#include "uistrings.h"


namespace EM
{

class StoredObjAccessData : public CallBacker
{ mODTextTranslationClass(StoredObjAccessData)
public:

			StoredObjAccessData(const MultiID&,
					    const SurfaceIODataSelection*);
			~StoredObjAccessData();

    bool		isErr() const	{ return !errmsg_.isEmpty(); }

    MultiID		key_;
    EM::EMObject*	obj_;
    Executor*		rdr_;
    Threads::Work*	work_;
    uiString		errmsg_;

    EMObject*		getEMObjFromEMM();
    void		workFinished(CallBacker*);

};

} // namespace EM


EM::EMObject* EM::StoredObjAccessData::getEMObjFromEMM()
{
    ObjectID objid = EMM().getObjectID( key_ );
    return objid < 0 ? 0 : EMM().getObject( objid );
}


EM::StoredObjAccessData::StoredObjAccessData( const MultiID& ky,
				    const EM::SurfaceIODataSelection* siods )
    : key_(ky)
    , obj_(0)
    , rdr_(0)
    , work_(0)
{
    EMObject* obj = getEMObjFromEMM();
    if ( obj && obj->isFullyLoaded() )
    {
	obj_ = obj; obj_->ref();
    }
    else
    {
	rdr_ = EMM().objectLoader( key_, siods );
	if ( !rdr_ )
	    { errmsg_ = tr("No loader for %1").arg(key_); return; }

	work_ = new Threads::Work( *rdr_, false );
	CallBack finishedcb( mCB(this,StoredObjAccessData,workFinished) );
	Threads::WorkManager::twm().addWork( *work_, &finishedcb );
    }
}


EM::StoredObjAccessData::~StoredObjAccessData()
{
    if ( obj_ )
	obj_->unRef();
    if ( work_ )
	Threads::WorkManager::twm().removeWork( *work_ );
    delete work_;
    delete rdr_;
}


void EM::StoredObjAccessData::workFinished( CallBacker* cb )
{
    const bool isfail = !Threads::WorkManager::twm().getWorkExitStatus( cb );
    if ( isfail )
    {
	if ( rdr_ )
	    errmsg_ = rdr_->uiMessage();
	if ( errmsg_.isEmpty() )
	    errmsg_ = tr("Error during background read");
    }
    else
    {
	EMObject* obj = getEMObjFromEMM();
	if ( !obj || !obj->isFullyLoaded() )
	    errmsg_ = tr("Could not get object from EMM");
	else
	    { obj_ = obj; obj_->ref(); }
    }
    delete work_; work_ = 0;
    delete rdr_; rdr_ = 0;
}



EM::StoredObjAccess::StoredObjAccess()
    : surfiodsel_(0)
{
}


EM::StoredObjAccess::StoredObjAccess( const MultiID& ky )
{
    add( ky );
}


EM::StoredObjAccess::~StoredObjAccess()
{
    delete surfiodsel_;
    deepErase( data_ );
}


void EM::StoredObjAccess::setLoadHint( const EM::SurfaceIODataSelection& sio )
{
    delete surfiodsel_;
    surfiodsel_ = new SurfaceIODataSelection( sio );
}


EM::StoredObjAccessData* EM::StoredObjAccess::get( const MultiID& ky )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	StoredObjAccessData* data = data_[idx];
	if ( data->key_ == ky )
	    return data;
    }
    return 0;
}


bool EM::StoredObjAccess::set( const MultiID& ky )
{
    StoredObjAccessData* data = get( ky );
    if ( data )
	data_ -= data;
    deepErase( data_ );
    if ( data )
	{ data_ += data; return true; }

    return add( ky );
}


bool EM::StoredObjAccess::add( const MultiID& ky )
{
    StoredObjAccessData* data = get( ky );
    if ( data )
	return true;

    StoredObjAccessData* newdata = new StoredObjAccessData( ky, surfiodsel_ );
    data_ += newdata;
    return !newdata->isErr();
}


void EM::StoredObjAccess::dismiss( const MultiID& ky )
{
    StoredObjAccessData* data = get( ky );
    if ( data )
    {
	data_ -= data;
	delete data;
    }
}


EM::EMObject* EM::StoredObjAccess::object( int iobj )
{
    return data_.validIdx(iobj) ? data_[iobj]->obj_ : 0;
}


const EM::EMObject* EM::StoredObjAccess::object( int iobj ) const
{
    return const_cast<EM::StoredObjAccess*>(this)->object( iobj );
}


bool EM::StoredObjAccess::isReady( int iobj ) const
{
    if ( iobj < 0 )
    {
	for ( int idx=0; idx<size(); idx++ )
	    if ( !isReady(idx) )
		return false;
	return true;
    }

    if ( !data_.validIdx(iobj) )
	return false;

    const StoredObjAccessData& data = *data_[iobj];
    return data.isErr() ? false : !data.rdr_;
}


bool EM::StoredObjAccess::isError( int iobj ) const
{
    if ( iobj < 0 )
    {
	for ( int idx=0; idx<size(); idx++ )
	    if ( isError(idx) )
		return true;
	return false;
    }

    return !data_.validIdx(iobj) || data_[iobj]->isErr();
}


float EM::StoredObjAccess::ratioDone( int iobj ) const
{
    if ( iobj < 0 )
    {
	const int sz = size();
	float done = 0.f;
	for ( int idx=0; idx<sz; idx++ )
	    done += ratioDone( idx );
	return sz < 1 ? done : done / sz;
    }

    if ( !data_.validIdx(iobj) || isError(iobj) )
	return 0.f;

    Executor* rdr = data_[iobj]->rdr_;
    if ( !rdr )
	return 1.0f;

    od_int64 totnr = rdr->totalNr();
    if ( totnr < 0 )
	return 0.5f;

    float ret = (float)rdr->nrDone();
    ret /= totnr;
    return ret;
}


uiString EM::StoredObjAccess::getError( int iobj ) const
{
    if ( iobj < 0 )
    {
	for ( int idx=0; idx<size(); idx++ )
	    if ( isError(idx) )
		return data_[idx]->errmsg_;
	return uiString::emptyString();
    }

    if ( !data_.validIdx(iobj) )
	return uiString::emptyString();

    return data_[iobj]->errmsg_;
}


bool EM::StoredObjAccess::finishRead()
{
    while ( !isReady() )
    {
	if ( isError() )
	    return false;
	Threads::sleep( 0.1 );
    }
    return true;
}


namespace EM
{

class StoredObjAccessReader : public ::Executor
{ mODTextTranslationClass(StoredObjAccessReader)
public:

StoredObjAccessReader( StoredObjAccess& oa )
    : ::Executor("Object Reader")
    , oa_(oa)
{
    msg_ = tr("Reading object data");
}

od_int64 totalNr() const	{ return 100; }
od_int64 nrDone() const		{ return mNINT64(oa_.ratioDone()*100.f); }
uiString uiMessage() const	{ return tr("Reading object data"); }
uiString uiNrDoneText() const	{ return uiStrings::sPercentageDone(); }

int nextStep()
{
    if ( oa_.isError() )
	{ msg_ = oa_.getError(); return ErrorOccurred(); }
    else if ( oa_.isReady() )
	return Finished();

    Threads::sleep( 0.1 );
    return MoreToDo();
}

    StoredObjAccess&	oa_;
    uiString		msg_;

};

} // namespace EM


Executor* EM::StoredObjAccess::reader()
{
    return new StoredObjAccessReader( *this );
}
