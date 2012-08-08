
/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : Jan 2009
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: emrandomposbody.cc,v 1.19 2012-08-08 05:47:55 cvssalil Exp $";

#include "emrandomposbody.h"

#include "ascstream.h"
#include "datapointset.h"
#include "embodyoperator.h"
#include "embodytr.h"
#include "emmanager.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "pickset.h"
#include "streamconn.h"
#include "survinfo.h"

namespace EM 
{

#define mRetErr( msg ) { errmsg_ = msg; return; }    
    
class RandomPosBodyReader : public Executor
{
public:

    RandomPosBodyReader( RandomPosBody& rdposbody, Conn* conn )
	: Executor( "RandomPos Loader" )
	, conn_( conn )
	, rdposbody_( rdposbody )
	, nrdone_( 0 )		       
	, totalnr_( -1 )
    {
	if ( !conn_ || !conn_->forRead() || !conn_->isStream() )
	    mRetErr( "Cannot open connection" );

	ascistream astream( ((StreamConn*)conn_)->iStream() );

	if ( !astream.stream().good() )
	    mRetErr( "Cannot read from input file" );
	if ( !astream.isOfFileType( sKeyRandomPosBodyFileType() ) )
	    mRetErr( sInvalidFile() );

	astream.next();

	if ( atEndOfSection( astream ) )
	    mRetErr( sInvalidFile() );

	IOPar iopar; iopar.getFrom( astream );
	if ( !rdposbody_.usePar( iopar ) )
	    mRetErr( sInvalidFile() );

	iopar.get( sKeyNrPositions(), totalnr_ );
    }

    int nextStep()
    { 
	if ( !errmsg_.isEmpty() )
	    return ErrorOccurred();

	std::istream& strm = ((StreamConn*)conn_)->iStream();
	if ( !strm.good() )
	{
	    rdposbody_.resetChangedFlag();
    	    return Finished();
	}

	Coord3 pos;

	const char* err = "Cannot interprete file";

	strm >> pos.x; 
	if ( strm.fail() )
	{
	    if ( !strm.good() )
	    {
		rdposbody_.resetChangedFlag();
		return Finished();
	    }
	    errmsg_=err;return ErrorOccurred();
	}

	strm >> pos.y; if ( strm.fail() ) {errmsg_=err;return ErrorOccurred();}
	strm >> pos.z; if ( strm.fail() ) {errmsg_=err;return ErrorOccurred();}
	int posid;
	strm >> posid; if ( strm.fail() ) {errmsg_=err;return ErrorOccurred();}

	if ( !strm.good() ) { errmsg_ = err; return ErrorOccurred(); }
	rdposbody_.addPos( pos );
	nrdone_++;

	return MoreToDo();
    }

    od_int64    	totalNr() const { return totalnr_; }
    od_int64    	nrDone() const	{ return nrdone_; }
    static const char*	sKeyRandomPosBodyFileType() 
    			{ return RandomPosBody::typeStr(); }
    static const char*	sKeyNrPositions() { return "Nr positions"; }

protected:

    static const char*		sInvalidFile() { return "Invalid file"; }

    RandomPosBody&		rdposbody_;
    Conn*			conn_;
    int				nrdone_;
    int				totalnr_;
    BufferString		errmsg_;
};    


class RandomPosBodyWriter : public Executor
{
public:

    RandomPosBodyWriter( RandomPosBody& rdposbody, Conn* conn )
	: Executor( "RandomPos Writer" )
	, conn_( conn )
	, rdposbody_( rdposbody )
	, nrdone_( 0 )		       
    {
	if ( !conn_->forWrite() || !conn_->isStream() )
	    mRetErr( "Internal error: bad connection" );
	
	ascostream astream( ((StreamConn*)conn_)->oStream() );
	astream.putHeader( RandomPosBody::typeStr() );
	if ( !astream.stream().good() )
	    mRetErr( "Cannot write to output Pick Set file" );

	IOPar pars;
	rdposbody.fillPar( pars );
	pars.set( RandomPosBodyReader::sKeyNrPositions(), totalNr() );
	pars.putTo( astream );
    }

    int nextStep()
    {
	if ( nrdone_>=totalNr() )
	{
	    ascostream astream( ((StreamConn*)conn_)->oStream() );
	    astream.newParagraph();
	    return Finished();
	}

	BufferString str;
	rdposbody_.getPositions()[nrdone_].fill( str.buf(),""," ","" );
	str += " ";
	const int idx = rdposbody_.posIDs()[nrdone_];
	if ( !idx ) 
	    str += "0";
	else
    	    str += idx;
	((StreamConn*)conn_)->oStream() << str.buf() << '\n';
	nrdone_++;	
	
	return MoreToDo();
    }

    od_int64    totalNr() const	{ return rdposbody_.getPositions().size(); }
    od_int64    nrDone() const	{ return nrdone_; }

protected:

    RandomPosBody&		rdposbody_;
    Conn*			conn_;
    int				nrdone_;
    BufferString		errmsg_;
};    


mImplementEMObjFuncs( RandomPosBody, randposEMBodyTranslator::sKeyUserName() ); 


RandomPosBody::RandomPosBody( EMManager& man )
    : EMObject( man )
{}


RandomPosBody::~RandomPosBody()
{}


MultiID RandomPosBody::storageID() const
{ return EMObject::multiID(); }


BufferString RandomPosBody::storageName() const
{ return EMObject::name(); }


void RandomPosBody::refBody()
{ EMObject::ref(); }


void RandomPosBody::unRefBody()
{ EMObject::unRef(); }


void RandomPosBody::copyFrom( const Pick::Set& ps )
{
    locations_.erase();
    ids_.erase();

    for ( int idx=0; idx<ps.size(); idx++ )
    {
	locations_ += ps[idx].pos;
	ids_ += idx;
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
	    ids_ += idx;
	}
    }
}


void RandomPosBody::copyFrom( const DataPointSet& data, int dpscolid,
			      const Interval<float>& valrg )
{
    locations_.erase();
    ids_.erase();

    if ( dpscolid<=0 || dpscolid>data.nrCols() )
	return;

    for ( int idx=0; idx<data.size(); idx++ )
    {
	if ( valrg.includes(data.value(dpscolid,idx),true) )
	{
	    locations_ += Coord3( data.coord(idx), data.z(idx) );
	    ids_ += idx;
	}
    }
}


void RandomPosBody::setPositions( const TypeSet<Coord3>& pts )
{
    locations_.erase();
    locations_ = pts;

    ids_.erase();
    for ( int idx=0; idx<pts.size(); idx++ )
       ids_ += idx;	
}


bool RandomPosBody::addPos( const Coord3& np )
{
    if ( locations_.indexOf(np)!=-1 )
	return false;

    locations_ += np;
    ids_ += ids_.size();

    return true;
}


Coord3 RandomPosBody::getPos( const PosID& posid ) const
{ return getPos( posid.sectionID(), posid.subID() ); }


Coord3 RandomPosBody::getPos( const SectionID& sid, const SubID& subid ) const
{
    if ( sid!=sectionID(0) )
	return Coord3::udf();

    const int posidx = ids_.indexOf( subid );
    return posidx==-1 ? Coord3::udf() : locations_[posidx];
}


bool RandomPosBody::setPos( const PosID& posid, const Coord3& pos, 
			    bool addtohistory )
{ return setPos( posid.sectionID(), posid.subID(), pos, addtohistory ); }


bool RandomPosBody::setPos( const SectionID& sid, const SubID& sub,
			    const Coord3& pos, bool addtohistory )
{
    if ( sid!=sectionID(0) || sub<0 )
	return false;

    if ( sub==ids_.size() )
	return addPos( pos );

    const int posidx = ids_.indexOf( sub );
    if ( posidx==-1 && sub==ids_.size() )
	return addPos( pos );
    else
	locations_[sub] = pos;

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
	myioobj = IOM().get( multiID() );
	ioobj = myioobj;
    }
    
    if ( !ioobj )
    {
	errmsg_ = "Cannot find surface";
	return 0;
    }

    Conn* conn = ioobj->getConn( Conn::Write );
    if ( !conn )    
     	return 0;

    return new RandomPosBodyWriter( *this, conn );
}


Executor* RandomPosBody::saver()
{ return saver( 0 ); }


Executor* RandomPosBody::loader()
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    if ( !ioobj )
	return 0;

    Conn* conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	return 0;
    
    return new RandomPosBodyReader( *this, conn );
}


const IOObjContext& RandomPosBody::getIOObjContext() const
{
    static IOObjContext* res = 0;
    if ( !res )
    {
	res = new IOObjContext(EMBodyTranslatorGroup::ioContext() );
	res->deftransl = randposEMBodyTranslator::sKeyUserName();
	res->toselect.allowtransls_ = randposEMBodyTranslator::sKeyUserName();
    }
    
    return *res;
}


ImplicitBody* RandomPosBody::createImplicitBody( TaskRunner* tr,
						 bool smooth ) const
{
    BodyOperator bodyopt;
    return bodyopt.createImplicitBody( locations_, tr ); 
}


bool RandomPosBody::getBodyRange( CubeSampling& cs )
{
    for ( int idx=0; idx<locations_.size(); idx++ )
    {
	cs.hrg.include( SI().transform(locations_[idx]) );

	if ( idx )
    	    cs.zrg.include( (float) locations_[idx].z );
	else
	    cs.zrg.start = cs.zrg.stop = (float) locations_[idx].z;
    }
    
    return locations_.size();
}



bool RandomPosBody::useBodyPar( const IOPar& par )
{
    if ( !EMObject::usePar( par ) )
	return false;

    ids_.erase();
    if ( !par.get( sKeySubIDs(), ids_ ) )
	return false;

    locations_.erase();
    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	Coord3 pos( Coord3::udf() );
       	BufferString skeypos("Location pos"); skeypos += ids_[idx]; 
	if (!par.get( skeypos.buf(), pos ) )
	    return false;

	locations_ += pos;
    }

    return true;
}


void RandomPosBody::fillBodyPar( IOPar& par ) const
{
    EMObject::fillPar( par );
    par.set( sKeySubIDs(), ids_ );
    for ( int idx=0; idx<locations_.size(); idx++ )
    {
       	BufferString skeypos("Location pos"); skeypos += ids_[idx]; 
	par.set( skeypos.buf(), locations_[idx] );
    }
}


}; // namespace EM
