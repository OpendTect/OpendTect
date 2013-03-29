/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellmarker.h"
#include "iopar.h"
#include "stratlevel.h"
#include "bufstringset.h"
#include "keystrs.h"

const char* Well::Marker::sKeyDah()	{ return "Depth along hole"; }


Well::Marker::Marker( int lvlid, float dh )
    : levelid_(lvlid)
    , dah_(dh)
{
}


Well::Marker::Marker( const Well::Marker& mrk )
{
    setName( mrk.name() );
    dah_ = mrk.dah();
    levelid_ = mrk.levelID();
    color_ = mrk.color();
}


const BufferString& Well::Marker::name() const
{
    return NamedObject::name();
}


Color Well::Marker::color() const
{
    return color_;
}


ObjectSet<Well::Marker>& Well::MarkerSet::operator += ( Well::Marker* mrk )
{
    if ( mrk && !isPresent( mrk->name().buf() ) )
	ObjectSet<Well::Marker>::operator += ( mrk );

    return *this;
}


Well::Marker* Well::MarkerSet::gtByName( const char* mname ) const
{
    const int idx = indexOf( mname );
    return  idx < 0 ? 0 : const_cast<Well::Marker*>((*this)[idx]); 
}


int Well::MarkerSet::indexOf( const char* mname ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( (*this)[idx]->name()==mname )
	    return idx;
    }
    return -1;
}


bool Well::MarkerSet::insertNew( Well::Marker* newmrk ) 
{
    if ( !newmrk || isPresent(newmrk->name().buf()) )
	{ delete newmrk; return false; }

    int newidx = 0;
    for ( int imrk=0; imrk<size(); imrk++ )
    {
	Well::Marker& mrk = *(*this)[imrk];
	if ( newmrk->dah() < mrk.dah() )
	    break;
	newidx++;
    }
    insertAt( newmrk, newidx );
    return true;
}


void Well::MarkerSet::append( const ObjectSet<Well::Marker>& ms )
{
    const size_type sz = ms.size();
    for ( size_type idx=0; idx<sz; idx++ )
    {
	if ( !isPresent(ms[idx]->name()) )
	    insertNew( new Well::Marker( *ms[idx] ) );
    }

}


Well::Marker* Well::MarkerSet::gtByLvlID(int lvlid) const
{
    if ( lvlid<=0 ) return 0;
    for ( int idmrk=0; idmrk<size(); idmrk++ )
    {
	Well::Marker* mrk = const_cast<Well::Marker*>((*this)[idmrk]); 
 	if ( mrk && mrk->levelID() == lvlid )
	    return mrk;
    }
    return 0;
}


void Well::MarkerSet::getNames( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<size(); idx++ )
	nms.add( (*this)[idx]->name() );
}


void Well::MarkerSet::getColors( TypeSet<Color>& cols ) const
{
    for ( int idx=0; idx<size(); idx++ )
	cols += (*this)[idx]->color();
}


void Well::MarkerSet::fillPar( IOPar& iop ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	IOPar mpar;
	const Marker& mrk = *(*this)[ idx ];
	mpar.set( sKey::Name(), mrk.name() );
	mpar.set( sKey::Color(), mrk.color() );
	mpar.set( sKey::Depth(), mrk.dah() );
	mpar.set( sKey::Level(), mrk.levelID() );
	iop.mergeComp( mpar, ::toString(idx+1) );
    }
}


void Well::MarkerSet::usePar( const IOPar& iop ) 
{
    setEmpty();

    for ( int imrk=1; ; imrk++ )
    {
	PtrMan<IOPar> mpar = iop.subselect( imrk );
	if ( !mpar || mpar->isEmpty() )
	    break;

	BufferString nm; mpar->get( sKey::Name(), nm );
	if ( nm.isEmpty() || isPresent(nm) )
	    continue;

	float dpt = 0; mpar->get( sKey::Depth(), dpt );
	Color col(0,0,0); mpar->get( sKey::Color(), col );
	int lvlid = -1; mpar->get( sKey::Level(), lvlid );

	Marker* mrk = new Marker( nm, dpt );
	mrk->setColor( col ); mrk->setLevelID( lvlid );
	insertNew( mrk );
    }
}


Well::MarkerRange::MarkerRange( const Well::MarkerSet& ms, Interval<int> rg )
    : markers_(ms)
    , rg_(rg)
{
    const int inpsz = ms.size();
    if ( inpsz < 1 )
	rg_.start = rg_.stop = -1;
    else
    {
	if ( rg_.start < 0 ) rg_.start = 0;
	if ( rg_.stop < 0 ) rg_.stop = inpsz - 1;
	rg_.sort();
	if ( rg_.start >= inpsz ) rg_.start = inpsz - 1;
	if ( rg_.stop >= inpsz ) rg_.stop = inpsz - 1;
    }
}


bool Well::MarkerRange::isValid() const
{
    const int inpsz = markers_.size();
    return inpsz > 0
	&& rg_.start >= 0 && rg_.stop >= 0
	&& rg_.start < inpsz && rg_.stop < inpsz
	&& rg_.start <= rg_.stop;
}


bool Well::MarkerRange::isIncluded( const char* nm ) const
{
    if ( !isValid() ) return false;

    for ( int idx=rg_.start; idx<=rg_.stop; idx++ )
	if ( markers_[idx]->name() == nm )
	    return true;
    return false;
}


bool Well::MarkerRange::isIncluded( float z ) const
{
    if ( !isValid() ) return false;

    return z >= markers_[rg_.start]->dah()
	&& z <= markers_[rg_.stop]->dah();
}


void Well::MarkerRange::getNames( BufferStringSet& nms ) const
{
    if ( !isValid() ) return;

    for ( int idx=rg_.start; idx<=rg_.stop; idx++ )
	nms.add( markers_[idx]->name() );
}


Well::MarkerSet* Well::MarkerRange::getResultSet() const
{
    Well::MarkerSet* ret = new Well::MarkerSet;
    if ( !isValid() ) return ret;

    for ( int idx=rg_.start; idx<=rg_.stop; idx++ )
	*ret += new Well::Marker( *markers_[idx] );
    return ret;
}
