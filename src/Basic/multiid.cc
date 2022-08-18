/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "multiid.h"

#include "perthreadrepos.h"
#include "separstr.h"
#include "survinfo.h"
#include "surveydisklocation.h"


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
    return groupID() > cFirstDatabaseGrpID();
}


bool MultiID::isInMemoryID() const
{
    return groupID() > 0 && groupID() <= cLastInMemoryGrpID();
}


bool MultiID::isTmpObjectID() const
{
    return isDatabaseID() && objectID() == cTmpObjID();
}


bool MultiID::isSyntheticID() const
{
    return isDatabaseID() && objectID() == cSyntheticObjID();
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
    const BufferString inpstr( str );
    const bool isoldmem = !inpstr.isEmpty() && inpstr.firstChar() == '#';
#ifdef __debug__
    if ( isoldmem )
	{ pErrMsg("Old format in-memory MultiID, adapt your code"); }
#endif

    const SeparString ss( isoldmem ? str+1 : str, '.' );
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


const SurveyDiskLocation& MultiID::surveyDiskLocation() const
{
    return SI().diskLocation();
}


void MultiID::setSurveyDiskLocation( const SurveyDiskLocation& sdl )
{
    pErrMsg("MultiID doesn't support this, please use DBKey instead");
}
