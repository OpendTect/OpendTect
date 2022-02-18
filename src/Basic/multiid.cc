/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		September 2021
________________________________________________________________________

-*/


#include "multiid.h"

#include "perthreadrepos.h"
#include "separstr.h"


static int sUdfID()	{ return -1; }

MultiID::MultiID()
    : MultiID(sUdfID(),sUdfID())
{}


MultiID::MultiID( int grpid, int objid )
{
    add( grpid ).add( objid );
}


MultiID::MultiID( const MultiID& mid )
{
    *this = mid;
}


MultiID::MultiID( const char* idstr )
{
    this->fromString( idstr );
}


MultiID::MultiID( int grpid, int objid, int subgrpid, int subobjid )
    : MultiID(grpid,objid)
{
    add( subgrpid ).add( subobjid );
}


MultiID::~MultiID()
{
}


MultiID& MultiID::setID( int idx, int id )
{
    if ( idx<0 || mIsUdf(idx) )
	return *this;

    while ( !ids_.validIdx(idx) )
	add( sUdfID() );

    ids_[idx] = id;
    return *this;
}


MultiID& MultiID::add( int id )
{
    ids_.add( id );
    return *this;
}


int MultiID::ID( int idx ) const
{
    return ids_.validIdx(idx) ? ids_[idx] : sUdfID();
}


MultiID MultiID::mainID() const
{
    return MultiID( groupID(), objectID() );
}


bool MultiID::isDatabaseID() const
{
    return groupID() > 100000;
}


const MultiID& MultiID::udf()
{
   static MultiID _udf;
   return _udf;
}


bool MultiID::isUdf() const
{
    return ids_.isEmpty() || (groupID()==sUdfID() && objectID()==sUdfID());
}


bool MultiID::fromString( const char* str )
{
    SeparString ss( str, '.' );
    if ( ss.isEmpty() )
    {
	setUdf();
	return false;
    }

    ids_.setEmpty();
    int nrids = ss.size();
    if ( nrids < 2 )
	nrids = 2;

    ids_.setSize( nrids, sUdfID() );
    for ( int idx=0; idx<ss.size(); idx++ )
	setID( idx, ss.getIValue(idx) );

    return true;
}


BufferString MultiID::toString() const
{
    SeparString ss;
    ss.setSepChar( '.' );
    for ( int idx=0; idx<ids_.size(); idx++ )
	ss.add( ids_[idx] );

    return ss.buf();
}


bool MultiID::isEqualTo( const char* idstr ) const
{
    return toString() == idstr;
}


MultiID& MultiID::operator =( const MultiID& mid )
{
    ids_ = mid.ids_;
    return *this;
}


bool MultiID::operator ==( const MultiID& mid ) const
{
    return ids_ == mid.ids_;
}


bool MultiID::operator !=( const MultiID& mid ) const
{
    return ids_ != mid.ids_;
}


int MultiID::leafID() const
{
    return ids_.last();
}


MultiID MultiID::parent() const
{
    return MultiID( ids_[0], ids_[1] );
}


const char* MultiID::buf() const
{
    mDeclStaticString( res );
    res = toString();
    return res.buf();
}
