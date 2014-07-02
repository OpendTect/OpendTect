/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "welllog.h"
#include "welllogset.h"
#include "iopar.h"
#include "idxable.h"
#include "unitofmeasure.h"

const char* Well::Log::sKeyUnitLbl()	{ return "Unit of Measure"; }
const char* Well::Log::sKeyHdrInfo()	{ return "Header info"; }
const char* Well::Log::sKeyStorage()	{ return "Storage type"; }


// ---- Well::LogSet


void Well::LogSet::getNames( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<logs.size(); idx++ )
	nms.add( logs[idx]->name() );
}


void Well::LogSet::add( Well::Log* wl )
{
    if ( !wl ) return;
    if ( getLog(wl->name()) ) return;

    logs += wl;
    updateDahIntv( *wl );
}


void Well::LogSet::updateDahIntv( const Well::Log& wl )
{
    if ( wl.isEmpty() ) return;

    if ( mIsUdf(dahintv.start) )
	{ dahintv.start = wl.dah(0); dahintv.stop = wl.dah(wl.size()-1); }
    else
    {
	if ( dahintv.start > wl.dah(0) )
	    dahintv.start = wl.dah(0);
	if ( dahintv.stop < wl.dah(wl.size()-1) )
	    dahintv.stop = wl.dah(wl.size()-1);
    }
}


void Well::LogSet::updateDahIntvs()
{
    for ( int idx=0; idx<logs.size(); idx++ )
    {
	logs[idx]->ensureAscZ();
	updateDahIntv( *logs[idx] );
    }
}


int Well::LogSet::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<logs.size(); idx++ )
    {
	const Log& l = *logs[idx];
	if ( l.name() == nm )
	    return idx;
    }
    return -1;
}


Well::Log* Well::LogSet::remove( int logidx )
{
    Log* log = logs[logidx]; logs -= log;
    ObjectSet<Well::Log> tmp( logs );
    logs.erase(); init();
    for ( int idx=0; idx<tmp.size(); idx++ )
	add( tmp[idx] );
    return log;
}


void Well::LogSet::setEmpty()
{
    deepErase( logs );
}


void Well::LogSet::removeTopBottomUdfs()
{
    for ( int idx=0; idx<logs.size(); idx++ )
	logs[idx]->removeTopBottomUdfs();
}


TypeSet<int> Well::LogSet::getSuitable( PropertyRef::StdType ptype,
	const PropertyRef* altpr, BoolTypeSet* arealt ) const
{
    TypeSet<int> ret;
    if ( arealt )
	arealt->setEmpty();

    for ( int idx=0; idx<logs.size(); idx++ )
    {
	const char* loguomlbl = logs[idx]->unitMeasLabel();
	const UnitOfMeasure* loguom = UnitOfMeasure::getGuessed( loguomlbl );
	bool isalt = false;
	bool isok = !loguom || ptype == PropertyRef::Other
	         || loguom->propType() == ptype;
	if ( !isok && altpr )
	    isok = isalt = loguom->propType() == altpr->stdType();
	if ( isok )
	{
	    ret += idx;
	    if ( arealt )
		*arealt += isalt;
	}
    }

    return ret;
}


// ---- Well::Log


Well::Log& Well::Log::operator =( const Well::Log& wl )
{
    if ( &wl != this )
    {
	setName( wl.name() );
	setUnitMeasLabel( wl.unitMeasLabel() );
	dah_ = wl.dah_; vals_ = wl.vals_; range_ = wl.range_;
	iscode_ = wl.iscode_;
    }
    return *this;
}


static bool valIsCode( float val, float eps )
{
    if ( mIsUdf(val) )
	return true; //No reason for failure

    return mIsEqual(val,mCast(float,mNINT32(val)),eps);
}


void Well::Log::setValue( int idx, float val )
{
    if ( !vals_.validIdx(idx) )
	return;

    vals_[idx] = val;
    if ( iscode_ && !valIsCode(val,1e-3f) )
	iscode_ = false;
}


float Well::Log::getValue( float dh, bool noudfs ) const
{
    if ( isEmpty() )
	return noudfs ? 0 : mUdf(float);

    int idx1;
    const float ret = gtVal( dh, idx1 );
    if ( !noudfs || !mIsUdf(ret) )
	return ret;

    float dah1=mUdf(float),val1=mUdf(float),dah2=mUdf(float),val2=mUdf(float);
    bool found1 = false, found2 = false;
    if ( idx1 > 0 )
    {
	for ( int idx=idx1; idx>=0; idx-- )
	{
	    const float val = value( idx );
	    if ( !mIsUdf(val) )
		{ dah1 = dah( idx ); val1 = val; found1 = true; break; }
	}
    }
    if ( idx1 < size()-1 )
    {
	for ( int idx=idx1+1; idx<size(); idx++ )
	{
	    const float val = value( idx );
	    if ( !mIsUdf(val) )
		{ dah2 = dah( idx ); val2 = val; found2 = true; break; }
	}
    }

    if ( !found1 && !found2 )
	return 0;
    else if ( !found1 )
	return val2;
    else if ( !found2 )
	return val1;

    if ( iscode_ )
	return val2;

    return ((dh-dah1) * val2 + (dah2-dh) * val1) / (dah2 - dah1);
}


float Well::Log::gtVal( float dh, int& idx1 ) const
{
    if ( IdxAble::findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return vals_[idx1];
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
	return mUdf(float);

    const int idx2 = idx1 + 1;
    const float v1 = vals_[idx1];
    const float v2 = vals_[idx2];

    const float d1 = dh - dah_[idx1];
    const float d2 = dah_[idx2] - dh;
    if ( iscode_ || mIsUdf(v1) || mIsUdf(v2) )
	return d1 > d2 ? v2 : v1;

    return ( d1*vals_[idx2] + d2*vals_[idx1] ) / (d1 + d2);
}


void Well::Log::addValue( float dh, float val )
{
    if ( !mIsUdf(val) )
    {
	if ( val < range_.start ) range_.start = val;
	if ( val > range_.stop ) range_.stop = val;
    }

    dah_ += dh;
    vals_ += val;
    if ( isEmpty() )
	iscode_ = valIsCode( val, 1e-3f );
    else if ( iscode_ && !valIsCode(val,1e-3f) )
	 iscode_ = false;
}


const UnitOfMeasure* Well::Log::unitOfMeasure() const
{
    return UnitOfMeasure::getGuessed(unitmeaslbl_);
}


void Well::Log::convertTo( const UnitOfMeasure* touom )
{
    const UnitOfMeasure* curuom = unitOfMeasure();
    if ( !curuom || !vals_.size() )
	return;

    for ( int idx=0; idx<vals_.size(); idx++ )
	convUserValue( vals_[idx], curuom, touom );

    if ( touom )
	unitmeaslbl_ = touom->symbol();
    else
    {
	PropertyRef::StdType tp = curuom->propType();
	const UnitOfMeasure* siuom = UoMR().getInternalFor( tp );
	unitmeaslbl_ = siuom ? siuom->symbol() : "";
    }
}


PropertyRef::StdType Well::Log::propType() const
{
    const UnitOfMeasure* uom = unitOfMeasure();
    return uom ? uom->propType() : PropertyRef::Other;
}


void Well::Log::updateAfterValueChanges()
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( !valIsCode(vals_[idx],1e-3f) )
	{
	    iscode_ = false;;
	    return;
	}
    }

    iscode_ = true;
}


void Well::Log::ensureAscZ()
{
    if ( dah_.size() < 2 ) return;
    const int sz = dah_.size();
    if ( dah_[0] < dah_[sz-1] ) return;
    const int hsz = sz / 2;
    for ( int idx=0; idx<hsz; idx++ )
    {
	Swap( dah_[idx], dah_[sz-idx-1] );
	Swap( vals_[idx], vals_[sz-idx-1] );
    }
}


void Well::Log::removeTopBottomUdfs()
{
    const int sz = size();
    Interval<int> defrg( 0, sz-1 );
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( !mIsUdf(vals_[idx]) )
	    break;
	defrg.start++;
    }
    for ( int idx=sz-1; idx>=defrg.start; idx-- )
    {
	if ( !mIsUdf(vals_[idx]) )
	    break;
	dah_.removeSingle( idx ); vals_.removeSingle( idx );
    }

    if ( defrg.start == 0 )
	return;

    TypeSet<float> newval, newdah;
    for ( int idx=defrg.start; idx<size(); idx++ )
	{ newdah += dah_[idx]; newval += vals_[idx]; }
    dah_ = newdah; vals_ = newval;
}


bool Well::Log::insertAtDah( float dh, float val )
{
    mWellDahObjInsertAtDah( dh, val, vals_, false );
    if ( val < range_.start ) range_.start = val;
    if ( val > range_.stop ) range_.stop = val;
    return true;
}
