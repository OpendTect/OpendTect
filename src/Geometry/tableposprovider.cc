/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2008
-*/


#include "tableposprovider.h"
#include "keystrs.h"
#include "picksetmanager.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "od_istream.h"
#include "iopar.h"
#include "ioobj.h"
#include "survinfo.h"
#include "survgeom.h"
#include "uistrings.h"
#include <math.h>

#define mGetTableKey(k) IOPar::compKey(sKey::Table(),k)


Pos::TableProvider3D::TableProvider3D()
    : Pos::Provider3D()
    , bvs_(1,true)
{}


Pos::TableProvider3D::TableProvider3D( const IOObj& ioobj )
    : Pos::Provider3D()
    , bvs_(1,true)
{
    IOPar iop; iop.set( mGetTableKey("ID"), ioobj.key() );
    usePar( iop );
}


Pos::TableProvider3D::TableProvider3D( const char* fnm )
    : Pos::Provider3D()
    , bvs_(1,true)
{
    IOPar iop; iop.set( mGetTableKey(sKey::FileName()), fnm );
    usePar( iop );
}


Pos::TableProvider3D::TableProvider3D( const Pos::TableProvider3D& oth )
    : Pos::Provider3D(oth)
    , bvs_(1,true)
{
    *this = oth;
}


Pos::TableProvider3D& Pos::TableProvider3D::operator =(
					const TableProvider3D& oth )
{
    if ( &oth == this )
	return *this;

    Pos::Provider3D::operator = ( oth );

    bvs_ = oth.bvs_;
    pos_ = oth.pos_;

    return *this;
}


const char* Pos::TableProvider3D::type() const
{
    return sKey::Table();
}


bool Pos::TableProvider3D::includes( const BinID& bid, float z ) const
{
    BinnedValueSet::SPos pos = bvs_.find( bid );
    if ( !pos.isValid() ) return false;
    if ( mIsUdf(z) ) return true;

    while ( true )
    {
	const float* val = bvs_.getVals( pos );
	if ( !val ) return false;
	const float zdiff = *val - z;
	if ( mIsZero(zdiff,mDefEps) )
	    return true;
	if ( !bvs_.next(pos,false) )
	    break;
	else if ( bvs_.getBinID(pos) != bid )
	    return false;
    }
    return false;
}


void Pos::TableProvider3D::getBVSFromPar( const IOPar& iop,
					  BinnedValueSet& bvs )
{
    const char* res = iop.find( mGetTableKey("ID") );
    if ( res && *res )
    {
	const DBKey dbky( res );
	ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( dbky );
	if ( ps )
	{
	    Pick::SetIter psiter( *ps );
	    const SurveyInfo& si = dbky.surveyInfo();
	    while ( psiter.next() )
	    {
		const Coord3 crd3 = psiter.get().pos();
		bvs.add( si.transform(crd3.getXY()), (float)crd3.z_ );
	    }
	}
	else
	{
	    PtrMan<IOObj> ioobj = getIOObj( dbky );
	    if ( ioobj && ioobj->group() == mTranslGroupName(PosVecDataSet) )
	    {
		::PosVecDataSet pvds; uiString errmsg;
		if ( pvds.getFrom(ioobj->mainFileName(),errmsg) )
		    bvs = pvds.data();
	    }
	}
    }

    if ( bvs.isEmpty() )
    {
	res = iop.find( mGetTableKey(sKey::FileName()) );
	if ( res && *res )
	{
	    od_istream strm( res );
	    if ( strm.isOK() )
		bvs.getFrom( strm, GeomID::get3D() );
	    strm.close();
	    if ( !bvs.isEmpty() )
	    {
		float zfac = -1;
		if ( !SI().zIsTime() )
		    zfac = SI().depthsInFeet() ? mFromFeetFactorF : -1;
		else if ( bvs.nrVals() > 0 )
		{
		    const Interval<float> zrg( bvs.valRange(0) );
		    if ( !mIsUdf(zrg.start) )
		    {
			const Interval<float> sizrg( SI().zRange() );
			const float siwdth = sizrg.width();
			if ( zrg.start < sizrg.start - 10 * siwdth
			  || zrg.stop > sizrg.stop + 10 * siwdth )
			    zfac = 0.001;
		    }
		}
		if ( zfac > 0 )
		{
		    BinnedValueSet::SPos p;
		    while ( bvs.next(p) )
		    {
			float* val = bvs.getVals( p );
			if ( !val ) break;
			*val *= zfac;
		    }
		}
	    }
	}
    }

    if ( bvs.isEmpty() )
	bvs.usePar( iop, mGetTableKey("Data") );
}



void Pos::TableProvider3D::usePar( const IOPar& iop )
{
    getBVSFromPar( iop, bvs_ );
}


void Pos::TableProvider3D::fillPar( IOPar& iop ) const
{
    bvs_.fillPar( iop, "Data" );
}


void Pos::TableProvider3D::getSummary( uiString& txt ) const
{
    const int sz = mCast( int, bvs_.totalSize() );
    if ( sz < 1 ) return;
    txt.appendPhrase(toUiString("%1 %2").arg(sz).arg(uiStrings::sPoint(sz)),
				    uiString::Space, uiString::OnSameLine);
    BinID start, stop;
    getExtent( start, stop );
    txt.appendPhrase( tr("Start/Stop at: %1-%2").arg(start.toString())
				.arg(stop.toString()), uiString::NoSep );
}


void Pos::TableProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    BinnedValueSet::SPos p; bvs_.next(p);
    if ( !p.isValid() )
	{ start = stop = BinID(0,0); return; }

    start = stop = bvs_.getBinID(p);
    while ( bvs_.next(p) )
    {
	const BinID bid( bvs_.getBinID(p) );
	if ( start.inl() > bid.inl() ) start.inl() = bid.inl();
	if ( stop.inl() < bid.inl() ) stop.inl() = bid.inl();
	if ( start.crl() > bid.crl() ) start.crl() = bid.crl();
	if ( stop.crl() < bid.crl() ) stop.crl() = bid.crl();
    }
}



void Pos::TableProvider3D::getZRange( Interval<float>& zrg ) const
{
    BinnedValueSet::SPos p; bvs_.next(p);
    if ( !p.isValid() )
	{ zrg.start = zrg.stop = 0; return; }

    const float* val = bvs_.getVals( p );
    if ( !val )
	{ zrg = SI().zRange(); return; }

    zrg.start = zrg.stop = *val;
    while ( bvs_.next(p) )
    {
	const float z = *bvs_.getVals(p);
	if ( zrg.start > z ) zrg.start = z;
	if ( zrg.stop < z ) zrg.stop = z;
    }
}


void Pos::TableProvider3D::initClass()
{
    Pos::Provider3D::factory().addCreator( create, sKey::Table(),
						    tr("Table","data table") );
}
