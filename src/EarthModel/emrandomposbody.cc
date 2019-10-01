
/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : Jan 2009
___________________________________________________________________

-*/


#include "emrandomposbody.h"

#include "ascstream.h"
#include "datapointset.h"
#include "embodyoperator.h"
#include "embodytr.h"
#include "emmanager.h"
#include "executor.h"
#include "ioobj.h"
#include "iopar.h"
#include "uistrings.h"
#include "pickset.h"
#include "streamconn.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "keystrs.h"

namespace EM
{

static uiString getErrStr( od_stream* strm, bool read )
{
    uiString errmsg = uiStrings::phrErrDuringIO(read);
    strm->addErrMsgTo( errmsg );
    return errmsg;
}

#define mRetErr( msg ) { errmsg_ = msg; return; }

class RandomPosBodyReader : public Executor
{ mODTextTranslationClass(EM::RandomPosBodyReader);
public:

    RandomPosBodyReader( RandomPosBody& rdposbody, Conn* conn )
	: Executor( "Random Position Body Loader" )
	, conn_( conn )
	, strm_( 0 )
	, rdposbody_( rdposbody )
	, nrdone_( 0 )
	, totalnr_( -1 )
    {
	if ( !conn_ || !conn_->forRead() || !conn_->isStream() )
	    mRetErr( mINTERNAL("bad connection for randompos body reader") );
	strm_ = &((StreamConn*)conn_)->iStream();

	ascistream astream( *strm_ );

	if ( !astream.isOK() )
	    mRetErr( uiStrings::phrCannotOpen(uiStrings::sFile()) );
	if ( !astream.isOfFileType( sFileType() ) &&
	     !astream.isOfFileType( sOldFileType()) )
	    mRetErr( sInvalidFile() );

	astream.next();

	if ( atEndOfSection( astream ) )
	    mRetErr( sInvalidFile() );

	IOPar iopar; iopar.getFrom( astream );
	if ( !rdposbody_.usePar( iopar ) )
	    mRetErr( sInvalidFile() );

	iopar.get( sKeyNrPositions(), totalnr_ );
    }

    uiString message() const
    {
	return !errmsg_.isEmpty() ? errmsg_
	     : uiStrings::phrReading(uiStrings::sPosition(mPlural));
    }
    uiString nrDoneText() const
    { return uiStrings::phrRead(uiStrings::sPosition(mPlural)); }

    int nextStep()
    {
	if ( !errmsg_.isEmpty() )
	    return ErrorOccurred();

	if ( !strm_->isOK() )
	{
	    rdposbody_.resetChangedFlag();
	    return Finished();
	}

	Coord3 pos;

	*strm_ >> pos.x_;
	if ( !strm_->isOK() )
	{
	    if ( !strm_->isBad() )
	    {
		rdposbody_.resetChangedFlag();
		return Finished();
	    }
	    errmsg_ = getErrStr( strm_, true );
	    return ErrorOccurred();
	}

	int posid;
	*strm_ >> pos.y_ >> pos.z_ >> posid;

	if ( !strm_->isOK() )
	    { errmsg_ = getErrStr( strm_, true ); return ErrorOccurred(); }

	rdposbody_.addPos( pos );
	nrdone_++;

	if ( nrdone_>=totalNr() )
	{
	    rdposbody_.resetChangedFlag();
	    return Finished();
	}

	return MoreToDo();
    }

    od_int64	totalNr() const		{ return totalnr_; }
    od_int64	nrDone() const		{ return nrdone_; }
    static const char*	sFileType()	{ return RandomPosBody::typeStr(); }
    static const char*	sOldFileType()	{ return "RandomPosBody"; }
    static const char*	sKeyNrPositions() { return "Nr positions"; }

protected:

    static uiString		sInvalidFile()
				{ return uiStrings::phrErrDuringRead(); }

    RandomPosBody&		rdposbody_;
    Conn*			conn_;
    od_istream*			strm_;
    int				nrdone_;
    int				totalnr_;
    uiString			errmsg_;

};


class RandomPosBodyWriter : public Executor
{
public:

    RandomPosBodyWriter( RandomPosBody& rdposbody, Conn* conn )
	: Executor( "Random Position Body Writer" )
	, conn_( conn )
	, rdposbody_( rdposbody )
	, nrdone_( 0 )
	, strm_( 0 )
    {
	if ( !conn_ || !conn_->forWrite() || !conn_->isStream() )
	    mRetErr( mINTERNAL("bad connection for random pos body writer") )
	strm_ = &((StreamConn*)conn_)->oStream();

	ascostream astream( *strm_ );
	astream.putHeader( RandomPosBody::typeStr() );
	if ( !astream.isOK() )
	    mRetErr( uiStrings::phrCannotWrite(uiStrings::sPointSet()) )

	IOPar pars;
	rdposbody.fillPar( pars );
	pars.set( RandomPosBodyReader::sKeyNrPositions(), totalNr() );
	pars.set( sKey::ZUnit(),
		  UnitOfMeasure::surveyDefZStorageUnit()->name() );

	pars.putTo( astream );
    }

    uiString message() const
    {
	return !errmsg_.isEmpty() ? errmsg_
	     : uiStrings::phrWriting(uiStrings::sPosition(mPlural));
    }
    uiString nrDoneText() const
    { return uiStrings::phrWritten(uiStrings::sPosition(mPlural)); }

    int nextStep()
    {
	if ( !strm_ || !errmsg_.isEmpty() )
	    return ErrorOccurred();

	if ( nrdone_>=totalNr() )
	{
	    ascostream astream( *strm_ );
	    astream.newParagraph();
	    return Finished();
	}

	const Coord3& crd( rdposbody_.getPositions()[nrdone_] );
	BufferString str;
	str.add( crd.x_ ).add( " " ).add( crd.y_ ).add( " " ).add( crd.z_ )
	    .add( " " ).add ( mCast(int,rdposbody_.posIDs()[nrdone_].getI()) );
	*strm_ << str.buf() << '\n';
	nrdone_++;

	if ( strm_->isBad() )
	    { errmsg_ = getErrStr( strm_, false ); return ErrorOccurred(); }
	return MoreToDo();
    }

    od_int64    totalNr() const	{ return rdposbody_.getPositions().size(); }
    od_int64    nrDone() const	{ return nrdone_; }

protected:

    RandomPosBody&	rdposbody_;
    Conn*		conn_;
    od_ostream*		strm_;
    int			nrdone_;
    uiString		errmsg_;

};


mImplementEMObjFuncs( RandomPosBody, "RandomPos" );


RandomPosBody::RandomPosBody( const char* nm )
    : Object(nm)
{}


RandomPosBody::~RandomPosBody()
{}


DBKey RandomPosBody::storageID() const
{ return Object::dbKey(); }


BufferString RandomPosBody::storageName() const
{ return Object::name(); }


void RandomPosBody::refBody()
{
    Object::ref();
}


void RandomPosBody::unRefBody()
{
    Object::unRef();
}


void RandomPosBody::copyFrom( const Pick::Set& ps )
{
    locations_.erase();
    ids_.erase();

    Pick::SetIter iter( ps );
    int idx = 0;
    while ( iter.next() )
    {
	locations_ += iter.get().pos();
	ids_ += PosID::get( idx );
	idx++;
    }
}


void RandomPosBody::copyFrom( const DataPointSet& data, int selgrp )
{
    locations_.erase();
    ids_.erase();

    for ( int idx=0; idx<data.size(); idx++ )
    {
	if ( selgrp<0 || data.selGroup(idx) == selgrp )
	{
	    locations_ += Coord3( data.coord(idx), data.z(idx) );
	    ids_ += PosID::get( idx );
	}
    }
}


void RandomPosBody::copyFrom( const DataPointSet& data, int dpscolid,
			      const Interval<float>& valrg )
{
    locations_.erase();
    ids_.erase();

    if ( dpscolid<0 || dpscolid>data.nrCols() )
	return;

    for ( int idx=0; idx<data.size(); idx++ )
    {
	if ( valrg.includes(data.value(dpscolid,idx),true) )
	{
	    locations_ += Coord3( data.coord(idx), data.z(idx) );
	    ids_ += PosID::get( idx );
	}
    }
}


void RandomPosBody::setPositions( const TypeSet<Coord3>& pts )
{
    locations_.erase();
    locations_ = pts;

    ids_.erase();
    for ( int idx=0; idx<pts.size(); idx++ )
       ids_ += PosID::get( idx );
}


bool RandomPosBody::addPos( const Coord3& np )
{
    if ( locations_.isPresent(np) )
	return false;

    locations_ += np;
    ids_ += PosID::get( ids_.size() );

    return true;
}


Coord3 RandomPosBody::getPos( const PosID& posid ) const
{
    const int posidx = ids_.indexOf( posid );
    return posidx==-1 ? Coord3::udf() : locations_[posidx];
}


bool RandomPosBody::setPosition( const PosID& sub,
				    const Coord3& pos, bool addtohistory,
				    NodeSourceType tp )
{
    if ( sub.getI()==ids_.size() )
	return addPos( pos );

    if ( !ids_.isPresent(sub) && sub.getI()==ids_.size() )
	return addPos( pos );
    else
	locations_[mCast(int,sub.getI())] = pos;

    return true;
}


Executor* RandomPosBody::saver( IOObj* inpioobj )
{
    PtrMan<IOObj> myioobj = 0;
    IOObj* ioobj = 0;
    if ( inpioobj )
	ioobj = inpioobj;
    else
    {
	myioobj = dbKey().getIOObj();
	ioobj = myioobj;
    }

    Conn* conn = ioobj ? ioobj->getConn( Conn::Write ) : 0;
    return conn ? new RandomPosBodyWriter( *this, conn ) : 0;
}


Executor* RandomPosBody::saver()
{ return saver(0); }


Executor* RandomPosBody::loader()
{
    PtrMan<IOObj> ioobj = dbKey().getIOObj();
    Conn* conn = ioobj ? ioobj->getConn( Conn::Read ) : 0;
    return conn ? new RandomPosBodyReader( *this, conn ) : 0;
}


bool RandomPosBody::isEmpty() const
{ return locations_.isEmpty(); }


const IOObjContext& RandomPosBody::getIOObjContext() const
{
    mDefineStaticLocalObject( PtrMan<IOObjContext>, res, = 0 );
    if ( !res )
    {
	IOObjContext* newres =
	    new IOObjContext(EMBodyTranslatorGroup::ioContext() );
	newres->fixTranslator( typeStr() );

	res.setIfNull(newres,true);
    }

    return *res;
}


ImplicitBody* RandomPosBody::createImplicitBody(
		const TaskRunnerProvider& trprov, bool smooth ) const
{
    BodyOperator bodyopt;
    return bodyopt.createImplicitBody( locations_, trprov );
}


bool RandomPosBody::getBodyRange( TrcKeyZSampling& cs )
{
    for ( int idx=0; idx<locations_.size(); idx++ )
    {
	cs.hsamp_.include( SI().transform(locations_[idx].getXY()) );

	if ( idx )
	    cs.zsamp_.include( (float) locations_[idx].z_ );
	else
	    cs.zsamp_.start = cs.zsamp_.stop = (float) locations_[idx].z_;
    }

    return locations_.size();
}



bool RandomPosBody::useBodyPar( const IOPar& par )
{
    if ( !Object::usePar( par ) )
	return false;

    ids_.erase();
    TypeSet<od_int64> idnums;
    if ( !par.get( sKeyPosIDs(), idnums ) )
	return false;

    for ( int idx=0; idx<idnums.size(); idx++ )
	ids_ += PosID::get( idnums[idx] );
    locations_.erase();
    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	Coord3 pos( Coord3::udf() );
	BufferString skeypos("Location pos"); skeypos += ids_[idx].getI();
	if (!par.get( skeypos.buf(), pos ) )
	    return false;

	locations_ += pos;
    }

    return true;
}


void RandomPosBody::fillBodyPar( IOPar& par ) const
{
    Object::fillPar( par );
    TypeSet<od_int64> idnums;
    for ( int idx=0; idx<ids_.size(); idx++ )
	idnums += ids_[idx].getI();

    par.set( sKeyPosIDs(), idnums );
    for ( int idx=0; idx<locations_.size(); idx++ )
    {
	BufferString skeypos("Location pos"); skeypos += ids_[idx].getI();
	par.set( skeypos.buf(), locations_[idx] );
    }
}


}; // namespace EM
