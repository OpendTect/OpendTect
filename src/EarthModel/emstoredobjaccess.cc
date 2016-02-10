/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "emstoredobjaccess.h"
#include "emmanager.h"
#include "emobject.h"
#include "executor.h"


namespace EM
{

class StoredObjAccessData
{ mODTextTranslationClass(StoredObjAccessData)
public:

		    StoredObjAccessData(const MultiID&);
		    ~StoredObjAccessData();

    bool	    isErr() const	{ return !errmsg_.isEmpty(); }

    MultiID	    key_;
    EM::EMObject*   obj_;
    Executor*	    rdr_;
    uiString	    errmsg_;

};

} // namespace EM


EM::StoredObjAccessData::StoredObjAccessData( const MultiID& ky )
    : key_(ky)
    , obj_(0)
    , rdr_(0)
{
    ObjectID objid = EMM().getObjectID( key_ );
    EMObject* obj = objid < 0 ? 0 : EMM().getObject( objid );
    if ( obj && obj->isFullyLoaded() )
    {
	obj_ = obj;
	obj_->ref();
    }
    else
    {
	rdr_ = EMM().objectLoader( key_ );
	if ( !rdr_ )
	    { errmsg_ = tr("No loader for %1").arg(key_); return; }

	//TODO get the rdr_ going
	//TODO and obj_?
    }
}


EM::StoredObjAccessData::~StoredObjAccessData()
{
    if ( obj_ )
	obj_->unRef();
}



EM::StoredObjAccess::StoredObjAccess()
{
}


EM::StoredObjAccess::StoredObjAccess( const MultiID& ky )
{
    add( ky );
}


EM::StoredObjAccess::~StoredObjAccess()
{
    deepErase( data_ );
}


bool EM::StoredObjAccess::add( const MultiID& ky )
{
    StoredObjAccessData* newdata = new StoredObjAccessData( ky );
    data_ += newdata;
    return !newdata->isErr();
}


void EM::StoredObjAccess::dismiss( const MultiID& ky )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	StoredObjAccessData* data = data_[idx];
	if ( data->key_ == ky )
	{
	    data_.removeSingle( idx );
	    delete data;
	    return;
	}
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
od_int64 nrDone() const		{ return mNINT64(oa_.ratioDone()/100.f); }
uiString uiMessage() const	{ return tr("Reading object data"); }
uiString uiNrDoneText() const	{ return tr("Percentage done"); }

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


Executor* EM::StoredObjAccess::saver( int iobj )
{
    //TODO
    return 0;
}
