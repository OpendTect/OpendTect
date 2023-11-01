/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "welltrack.h"

#include "idxable.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "velocitycalc.h"
#include "timedepthmodel.h"
#include "welldata.h"
#include "trigonometry.h"

#include "hiddenparam.h"

static HiddenParam<Well::Track,TypeSet<Coord3>* > welltrackzposmgr_(nullptr);

Well::Track::Track( const char* nm )
    : DahObj(nm)
    , zistime_(false)
{
    welltrackzposmgr_.setParam( this, nullptr );
}


Well::Track::Track( const Track& oth )
    : DahObj("")
{
    welltrackzposmgr_.setParam( this, nullptr );
    *this = oth;
}


Well::Track::~Track()
{
    welltrackzposmgr_.removeAndDeleteParam( this );
}


const TypeSet<Coord3>* Well::Track::zpos_() const
{
    return welltrackzposmgr_.getParam( this );
}


TypeSet<Coord3>* Well::Track::zpos_()
{
    return welltrackzposmgr_.getParam( this );
}


void Well::Track::removeZPos()
{
    welltrackzposmgr_.deleteAndNullPtrParam( this );
}


Well::Track& Well::Track::operator =( const Track& oth )
{
    if ( &oth != this )
    {
	DahObj::operator=( oth );
	pos_ = oth.pos_;
	zistime_ = oth.zistime_;
	removeZPos();
	if ( oth.zpos_() )
	    welltrackzposmgr_.setParam( this,
					new TypeSet<Coord3>(*oth.zpos_()));
    }

    return *this;
}


bool Well::Track::isEmpty() const
{
    return dah_.isEmpty() || pos_.isEmpty();
}


float Well::Track::getKbElev() const
{
    if ( isEmpty() )
	return 0;

    return dah_[0] - value(0);
}


const Interval<double> Well::Track::zRangeD() const
{
    if ( isEmpty() )
	return Interval<double>( 0., 0. );

    double zstart = pos_[0].z;
    double zstop = pos_[0].z;

    for ( int idx=1; idx<size(); idx++ )
    {
	const double zval = pos_[idx].z;
	if ( zval < zstart )
	    zstart = zval;
	else if ( zval > zstop )
	    zstop = zval;
    }

    return Interval<double> ( zstart, zstop );
}


const Interval<float> Well::Track::zRange() const
{
    Interval<double> zrange = zRangeD();
    return Interval<float> ( (float) zrange.start, (float) zrange.stop );
}


void Well::Track::addPoint( const Coord3& c, float dahval )
{
    pos_ += c;
    if ( mIsUdf(dahval) )
    {
	const int previdx = dah_.size() - 1;
	dahval = previdx < 0 && previdx < pos_.size()-1 ? 0.f
	    : mCast(float,pos_[previdx].distTo(pos_[previdx+1])+dah_[previdx] );
    }

    dah_ += dahval;
}


void Well::Track::addPoint( const Coord& c, float z, float dahval )
{
    Coord3 c3( c, z );
    addPoint( c3, dahval );
}


void Well::Track::insertAfterIdx( int aftidx, const Coord3& c )
{
    const int oldsz = pos_.size();
    if ( aftidx > oldsz-2 )
	{ addPoint( c ); return; }

    double extradah, owndah = 0;
    if ( aftidx == -1 )
	extradah = c.distTo( pos_[0] );
    else
    {
	double dist0 = c.distTo( pos_[aftidx] );
	double dist1 = c.distTo( pos_[aftidx+1] );
	owndah = dah_[aftidx] + dist0;
	extradah = dist0 + dist1 - pos_[aftidx].distTo( pos_[aftidx+1] );
    }

    pos_.insert( aftidx+1, c );
    dah_.insert( aftidx+1, mCast(float,owndah) );
    addToDahFrom( aftidx+2, mCast(float,extradah) );
}


int Well::Track::insertPoint( const Coord3& c )
{
    const int oldsz = pos_.size();
    if ( oldsz < 1 )
	{ addPoint( c ); return oldsz; }

    Coord3 cnew( c );
    if ( oldsz < 2 )
    {
	Coord3 oth( pos_[0] );
	if ( oth.z < cnew.z )
	{
	    addPoint( c );
	    return oldsz;
	}
	else
	{
	    pos_.erase(); dah_.erase();
	    pos_ += cnew; pos_ += oth;
	    dah_ += 0.f;
	    dah_ += mCast(float,oth.distTo( cnew ));
	    return 0;
	}
    }

    // Need to find 'best' position. This is when the angle of the triangle
    // at the new point is maximal
    // This boils down to min(sum of sq distances / product of distances)

    double minval = 1e30; int minidx = -1;
    int mindistidx = 0;
    double mindist = pos_[mindistidx].distTo(cnew);
    for ( int idx=1; idx<oldsz; idx++ )
    {
	const Coord3& c0 = pos_[idx-1];
	const Coord3& c1 = pos_[idx];
	const double d = c0.distTo( c1 );
	const double d0 = c0.distTo( cnew );
	const double d1 = c1.distTo( cnew );
	if ( mIsZero(d0,1e-4) || mIsZero(d1,1e-4) )
	    return -1; // point already present
	double val = (( d0 * d0 + d1 * d1 - ( d * d ) ) / (2 * d0 * d1));
	if ( val < minval )
	    { minidx = idx-1; minval = val; }
	if ( d1 < mindist )
	    { mindist = d1; mindistidx = idx; }
	if ( idx == oldsz-1 && minval > 0 )
	{
	    if ( mindistidx == oldsz-1)
	    {
		addPoint( c );
		return oldsz;
	    }
	    else if ( mindistidx > 0 && mindistidx < oldsz-1 )
	    {
		double prevdist = pos_[mindistidx-1].distTo(cnew);
		double nextdist = pos_[mindistidx+1].distTo(cnew);
		minidx = prevdist > nextdist ? mindistidx : mindistidx -1;
	    }
	    else
		minidx = mindistidx;
	}
    }

    if ( minidx == 0 )
    {
	// The point may be before the first
	const Coord3& c0 = pos_[0];
	const Coord3& c1 = pos_[1];
	const double d01sq = c0.sqDistTo( c1 );
	const double d0nsq = c0.sqDistTo( cnew );
	const double d1nsq = c1.sqDistTo( cnew );
	if ( d01sq + d0nsq < d1nsq )
	    minidx = -1;
    }
    if ( minidx == oldsz-2 )
    {
	// Hmmm. The point may be beyond the last
	const Coord3& c0 = pos_[oldsz-2];
	const Coord3& c1 = pos_[oldsz-1];
	const double d01sq = c0.sqDistTo( c1 );
	const double d0nsq = c0.sqDistTo( cnew );
	const double d1nsq = c1.sqDistTo( cnew );
	if ( d01sq + d1nsq < d0nsq )
	    minidx = oldsz-1;
    }

    insertAfterIdx( minidx, cnew );
    return minidx+1;
}


int  Well::Track::insertPoint( const Coord& c, float z )
{
    Coord3 c3( c, z );
    return insertPoint( c3 );
}


bool Well::Track::insertAtDah( float dh, float zpos )
{
    if ( dah_.isEmpty() )
	return false;

    if ( dh < dah_[0] )
    {
	dah_.insert( 0, dh );
	Coord3 crd( pos_[0] ); crd.z = mCast(double,zpos);
	pos_.insert( 0, crd );
	return true;
    }
    if ( dh > dah_[size()-1] )
    {
	dah_ += dh;
	Coord3 crd( pos_[size()-1] ); crd.z = zpos;
	pos_ += crd;
	return true;
    }

    const int insertidx = indexOf( dh );
    if ( insertidx<0 )
	return false;

    if ( mIsEqual(dh,dah_[insertidx],1e-3) )
	return true;

    Coord3 prevcrd( pos_[insertidx] );
    Coord3 nextcrd( pos_[insertidx+1] );
    Coord3 crd( ( prevcrd + nextcrd )/2 );
    crd.z = zpos;

    dah_.insert( insertidx+1, dh );
    pos_.insert( insertidx+1, crd );

    return true;
}


void Well::Track::setPoint( int idx, const Coord3& c )
{
    const int nrpts = pos_.size();
    if ( idx<0 || idx>=nrpts ) return;

    Coord3 oldpt( pos_[idx] );
    Coord3 newpt( c );
    double olddist0 = idx > 0 ? oldpt.distTo(pos_[idx-1]) : 0;
    double newdist0 = idx > 0 ? newpt.distTo(pos_[idx-1]) : 0;
    double olddist1 = 0, newdist1 = 0;
    if ( idx < nrpts-1 )
    {
	olddist1 = oldpt.distTo(pos_[idx+1]);
	newdist1 = newpt.distTo(pos_[idx+1]);
    }

    pos_[idx] = newpt;
    dah_[idx] += mCast( float, newdist0 - olddist0 );
    const float dist = mCast(float, newdist0 - olddist0 + newdist1 - olddist1 );
    addToDahFrom( idx+1, dist );
}


void Well::Track::setPoint( int idx, const Coord& c, float z )
{
    Coord3 c3( c, z );
    setPoint( idx, c3 );
}


void Well::Track::removePoint( int idx )
{
    if ( idx < pos_.size()-1 && idx < dah_.size()-1 )
    {
	float olddist = idx ? dah_[idx+1] - dah_[idx-1] : dah_[1];
	float newdist = idx ? (float) pos_[idx+1].distTo( pos_[idx-1] ) : 0;
	float extradah = olddist - newdist;
	removeFromDahFrom( idx+1, extradah );
    }

    remove( idx );
}


Coord3 Well::Track::getPos( float dh ) const
{
    if ( pos_.isEmpty() )
	return mUdf(Coord3);

    int idx1;
    const int tracksz = dah_.size();
    if ( IdxAble::findFPPos(dah_,tracksz,dh,-1,idx1) )
	return pos_[idx1];
    else if ( idx1 >= 0 && idx1 < tracksz-1 )
	return coordAfterIdx( dh, idx1 );

    Coord3 ret;
    if ( idx1 < 0 )
    {
	double deltamd = dah_[0] - dh;
	if ( tracksz < 2 || zistime_ )
	{
	    ret = pos_[0];
	    if ( tracksz > 1 && zistime_ )
	    {
		const double grad = ( pos_[1].z - pos_[0].z ) /
				    ( dah_[1]   - dah_[0] );
		deltamd *= grad;
	    }
	    ret.z -= deltamd;
	}
	else
	{
	    const double firstmddiff = dah_[1] - dah_[0];
	    const Coord3& firstpos = pos_[0];
	    const Coord3& secpos = pos_[1];
	    const Coord3 posdiff = secpos - firstpos;
	    ret = firstpos - posdiff * deltamd/firstmddiff;
	}
    }
    else
    {
	double deltamd = dh - dah_[tracksz-1];
	if ( tracksz < 2 || zistime_ )
	{
	    ret = pos_[tracksz-1];
	    if ( tracksz > 1 && zistime_ )
	    {
		const double grad = ( pos_[idx1].z - pos_[idx1-1].z ) /
				    ( dah_[idx1]   - dah_[idx1-1] );
		deltamd *= grad;
	    }
	    ret.z += deltamd;
	}
	else
	{
	    const double lastmddiff = dah_[tracksz-1] - dah_[tracksz-2];
	    const Coord3& lastpos = pos_[tracksz-1];
	    const Coord3& seclastpos = pos_[tracksz-2];
	    const Coord3 posdiff = lastpos - seclastpos;
	    ret = lastpos + posdiff * deltamd/lastmddiff;
	}
    }

    return ret;
}


float Well::Track::getDepth( const Well::Data& wd, float dah,
			     Well::Info::DepthType dtyp )
{
    const Well::Track& trk = wd.track();
    const Well::D2TModel* d2t = wd.d2TModel();
    const float kb = trk.getKbElev();
    const float srd = SI().seismicReferenceDatum();
    const float gl = wd.info().groundelev_;
    const float zpos = trk.getPos(dah).z;
    const float z = dtyp==Well::Info::MD ? dah :
		    dtyp==Well::Info::TVD ? zpos+kb :
		    dtyp==Well::Info::TVDSS ? zpos :
		    dtyp==Well::Info::TVDSD ? zpos+srd :
		    dtyp==Well::Info::TVDGL ? zpos+gl :
		    d2t ? d2t->getTime(dah,trk) : mUdf(float);

    return z;
}


mDefParallelCalc6Pars(Dah2Tvd,
		  od_static_tr("Dah2Tvd", "Dah to TVD conversion"),
		      const float*, daharr, const Well::Track&, track,
		      const UnitOfMeasure*, dah_uom, float*, tvdsarr,
		      float, zshft, const UnitOfMeasure*, tvd_uom)
mDefParallelCalcBody(
const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
,
const float dah =  getConvertedValue( daharr_[idx], dah_uom_, zsuom );
const float tvd = mIsUdf(dah) ? mUdf(float) : track_.getPos(dah).z + zshft_;
tvdsarr_[idx] = getConvertedValue( tvd, zsuom, tvd_uom_ );
,
)


void Well::Track::getAllTVD( int sz, const float* daharr,
			     const UnitOfMeasure* dah_uom, float* tvdsarr,
			     const UnitOfMeasure* tvd_uom,
			     Well::Info::DepthType dtype ) const
{
    const float zshft = dtype==Well::Info::TVD ? getKbElev() :
			dtype==Well::Info::TVDSS ? 0.f :
			dtype==Well::Info::TVDSD ?
					    SI().seismicReferenceDatum() : 0.f;

    Dah2Tvd converter( sz, daharr, *this, dah_uom, tvdsarr, zshft, tvd_uom );
    converter.execute();
}


Interval<float> Well::Track::getTVDRange( const Interval<float>& dahrg,
					  const UnitOfMeasure* in_uom,
					  const UnitOfMeasure* out_uom,
					  Well::Info::DepthType dtype ) const
{
    const float zshft = dtype==Well::Info::TVD ? getKbElev() :
			dtype==Well::Info::TVDSS ? 0.f :
			dtype==Well::Info::TVDSD ?
					    SI().seismicReferenceDatum() : 0.f;
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();

    const float start = getConvertedValue( dahrg.start, in_uom, zsuom );
    const float stop = getConvertedValue( dahrg.stop, in_uom, zsuom );
    const float tvdbeg = getConvertedValue( getPos(start).z+zshft, zsuom,
					    out_uom );
    const float tvdend = getConvertedValue( getPos(stop).z+zshft, zsuom,
					    out_uom );
    return Interval<float>( tvdbeg, tvdend );
}


Coord3 Well::Track::coordAfterIdx( float dh, int idx1 ) const
{
    const int idx2 = idx1 + 1;
    const double d1 = (double)( dh - dah_[idx1] );
    const double d2 = (double)( dah_[idx2] - dh );
    const Coord3& c1 = pos_[idx1];
    const Coord3& c2 = pos_[idx2];
    const double f =  1. / (d1 + d2);
    return Coord3( f * ( d1 * c2.x + d2 * c1.x ),
		   f * ( d1 * c2.y + d2 * c1.y ),
		   f * ( d1 * c2.z + d2 * c1.z ) );
}


float Well::Track::getDahForTVD( double z, float prevdah ) const
{
    const bool haveprevdah = !mIsUdf(prevdah);
    const int sz = dah_.size();
    if ( sz < 1 )
	return mUdf(float);

    if ( zistime_ )
    {
	pErrMsg("getDahForTVD called for time well");
	const float res = haveprevdah ? prevdah : dah_[0];
	return res;
    }

    static const double eps = 1e-3; // do not use lower for float precision
    static const double epsf = 1e-3f; // do not use lower for float precision
    if ( sz == 1 )
	return mIsEqual(z,pos_[0].z,eps) ? dah_[0] : mUdf(float);

    const Interval<double> zrange = zRangeD();
    if ( !zrange.includes(z,false) )
    {
	if ( z < pos_[0].z && dah_[0] > epsf )
	{
	    const float retdah = z + getKbElev();
	    return retdah > -1*epsf ? retdah : mUdf(float);
	}

	return mUdf(float);
    }

#define mZInRg() \
    (zrg.start-eps < z  && zrg.stop+eps  > z) \
 || (zrg.stop-eps  < z  && zrg.start+eps > z)

    Interval<double> zrg( zrange.start, 0 );
    int idxafter = -1;
    for ( int idx=1; idx<sz; idx++ )
    {
	if ( !haveprevdah || prevdah+epsf < dah_[idx] )
	{
	    zrg.stop = pos_[idx].z;
	    if ( mZInRg() )
		{ idxafter = idx; break; }
	}
	zrg.start = zrg.stop;
    }
    if ( idxafter < 1 )
	return mUdf(float);

    const int idx1 = idxafter - 1;
    const int idx2 = idxafter;
    const double z1 = pos_[idx1].z;
    const double z2 = pos_[idx2].z;
    const double dah1 = mCast(double,dah_[idx1]);
    const double dah2 = mCast(double,dah_[idx2]);
    const double zdiff = z2 - z1;
    const double res = ( (z-z1) * dah2 + (z2-z) * dah1 ) / zdiff;
    return mIsZero(zdiff,eps) ? dah_[idx2] : mCast( float, res );
}


float Well::Track::getDahForTVD( float z, float prevdah ) const
{
    return getDahForTVD( (double)z, prevdah );
}


#define mSimpleWellVel 2000.

float Well::Track::nearestDah( const Coord3& posin ) const
{
    if ( dah_.isEmpty() )
	return 0;
    if ( dah_.size() < 2 )
	return dah_[0];

    const double zfac = zistime_ ? mSimpleWellVel : 1.;
    Coord3 curpos = posin;
    curpos.z *= zfac;
    if ( pos_.size() > 1000 )
    {
	const TypeSet<Coord3>& input_pos = zistime_ ? *zpos_() : pos_;
	NearestCoordFinder finder( input_pos, curpos );
	if ( finder.execute() && finder.nearestIndex()!=-1 )
	    return dah_[finder.nearestIndex()];
    }

    int startidx = 0;
    Coord3 actualboundstart = getPos( dah_[startidx] );
    actualboundstart.z *= zfac;
    Coord3 actualboundstop = getPos( dah_[startidx+1] );
    actualboundstop.z *= zfac;
    Coord3 curposonline;
    for ( int idx=0; idx<dah_.size()-1; idx++ )
    {
	Coord3 boundposstart = getPos( dah_[idx] ); boundposstart.z *= zfac;
	Coord3 boundposstop = getPos( dah_[idx+1] ); boundposstop.z *= zfac;
	Vector3 dir = boundposstop-boundposstart;
	Line3 newline(boundposstop,dir);

	Interval<float> zintrvl( mCast(float,boundposstart.z),
				       mCast(float,boundposstop.z) );
	if ( zintrvl.includes(curpos.z,true) )
	{
	    Coord3 posonline = newline.getPoint(newline.closestPoint(curpos));
	    if ( posonline.isDefined() )
	    {
		curposonline = posonline;
		actualboundstart = boundposstart;
		actualboundstop = boundposstop;
		startidx = idx;
	    }
	}
    }

    double sqrdisttostart = actualboundstart.sqDistTo( curposonline );
    double sqrdisttostop = actualboundstop.sqDistTo( curposonline );

    const double distfrmstart = Math::Sqrt( sqrdisttostart );
    const double disttoend = Math::Sqrt( sqrdisttostop );
    const double dahnear = mCast(double,dah_[startidx]);
    const double dahsec = mCast(double,dah_[startidx+1]);
    double res = ( distfrmstart*dahsec+disttoend*dahnear )/
					  ( distfrmstart+disttoend );
    return mCast( float, res );
}


bool Well::Track::alwaysDownward() const
{
    if ( size() < 2 )
	return size();

    double prevz = pos_[0].z;
    for ( int idx=1; idx<pos_.size(); idx++ )
    {
	double curz = pos_[idx].z;
	if ( curz <= prevz )
	    return false;

	prevz = curz;
    }

    return true;
}


bool Well::Track::extendIfNecessary( const Interval<float>& dahrg )
{
    const int tracksz = size();
    if ( tracksz < 2 || zIsTime() )
	return false;

    Interval<float> newdahrg( dahrg );
    if ( mIsUdf(newdahrg.start) || mIsUdf(newdahrg.stop) )
	return false;
    else if ( newdahrg.start < 0.f )
	newdahrg.start = 0.f;

    const Interval<float> trackrg = dahRange();
    if ( mIsUdf(trackrg.start) || mIsUdf(trackrg.stop) ||
	 (newdahrg.start+1e-2f > trackrg.start &&
	  newdahrg.stop-1e-2f < trackrg.stop) )
	return false;

    bool updated = false;
    if ( newdahrg.start < trackrg.start )
    {
	pos_.insert( 0, getPos( newdahrg.start ) );
	dah_.insert( 0, newdahrg.start );
	updated = true;
    }

    if ( newdahrg.stop > trackrg.stop )
    {
	addPoint( getPos( newdahrg.stop ), newdahrg.stop );
	updated = true;
    }

    if ( updated )
	updateDahRange();

    return updated;
}


#define cDistTol 0.5f
void Well::Track::toTime( const Data& wd )
{
    const Track& track = wd.track();
    const D2TModel* d2t = wd.d2TModel();
    if ( track.isEmpty() )
	return;

    const UnitOfMeasure* depthstoruom =
			 UnitOfMeasure::surveyDefDepthStorageUnit();
    const UnitOfMeasure* depthuom = UnitOfMeasure::surveyDefDepthUnit();

    TimeDepthModel replvelmodel;
    const float srddepth = getConvertedValue(
				-1.f * SI().seismicReferenceDatum(),
				UnitOfMeasure::surveyDefSRDStorageUnit(),
				depthuom );
    const float dummythickness = 1000.f;
    TypeSet<float> replveldepths, replveltimes;
    replveldepths += srddepth - dummythickness;
    replveltimes += -2.f * dummythickness /
	    getConvertedValue( wd.info().replvel_, depthstoruom, depthuom );
    replveldepths += srddepth;
    replveltimes += 0.f;
    replvelmodel.setModel( replveldepths.arr(), replveltimes.arr(),
			   replveldepths.size() );

    TimeDepthModel dtmodel;
    if ( d2t && !d2t->getTimeDepthModel(wd,dtmodel) )
	return;

    TypeSet<float> newdah;
    TypeSet<Coord3> newpos;
    newdah += track.dah( 0 );
    newpos += track.pos( 0 );

    float prevdah = newdah[0];
    for ( int trckidx=1; trckidx<track.size(); trckidx++ )
    {
	const float curdah = track.dah( trckidx );
	const float dist = curdah - prevdah;
	if ( dist > cDistTol )
	{
	    const int nrchunks = mCast( int, dist / cDistTol );
	    const float step = dist / mCast(float, nrchunks );
	    StepInterval<float> dahrange( prevdah, curdah, step );
	    for ( int idx=1; idx<dahrange.nrSteps(); idx++ )
	    {
		const float dahsegment = dahrange.atIndex( idx );
		newdah += dahsegment;
		newpos += track.getPos( dahsegment );
	    }
	}

	newdah += curdah;
	newpos += track.pos( trckidx );
	prevdah = curdah;
    }

    // Copy the extended set into the new track definition
    dah_ = newdah;
    pos_ = newpos;
    // Now, convert to time
    for ( int idx=0; idx<dah_.size(); idx++ )
    {
	double& depth = pos_[idx].z;
	const bool abovesrd = depth < srddepth;
	if ( !abovesrd && !d2t )
	{
	    depth = mUdf(double);
	    continue; //Should never happen
	}

	convValue( depth, depthstoruom, depthuom );
	depth = mCast( double, abovesrd ? replvelmodel.getTime( (float)depth )
					: dtmodel.getTime( (float)depth ) );
    }

    removeZPos();
    auto* zpos = new TypeSet<Coord3>( pos_ );
    welltrackzposmgr_.setParam( this, zpos );
    for ( int idz=0; idz<pos_.size(); idz++ )
	(*zpos)[idz].z *= mSimpleWellVel;

    zistime_ = true;
}
