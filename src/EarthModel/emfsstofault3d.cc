/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "emfsstofault3d.h"

#include "survinfo.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "sorting.h"
#include "trigonometry.h"


namespace  EM
{
#define mOnInline 0
#define mOnCrlline 1
#define mOnZSlice 2
#define mOnOther 3


FSStoFault3DConverter::FaultStick::FaultStick( int sticknr )
	: sticknr_(sticknr)
	, pickedonplane_(false)
	, normal_(Coord3::udf())
{}


double FSStoFault3DConverter::FaultStick::slope( double zscale ) const
{
    if ( crds_.size()<2 )
	return mUdf(double);

    double basedist = Coord(crds_[0]).distTo( crds_[crds_.size()-1] );
    if ( mIsZero(basedist,mDefEps) )
	return MAXDOUBLE;

    return fabs( zscale*(crds_[0].z-crds_[crds_.size()-1].z) / basedist );
}


Coord3 FSStoFault3DConverter::FaultStick::findPlaneNormal() const
{
    const int maxdist = 5;
    int oninl = 0; int oncrl = 0; int ontms = 0;

    for ( int idx=0; idx<crds_.size()-1; idx++ )
    {
	const BinID bid0 = SI().transform( crds_[idx] );
	for ( int idy=idx+1; idy<crds_.size(); idy++ )
	{
	    const BinID bid1 = SI().transform( crds_[idy] );
	    const int inldist = abs( bid0.inl-bid1.inl );
	    if ( inldist < maxdist )
		oninl += maxdist - inldist;
	    const int crldist = abs( bid0.crl-bid1.crl );
	    if ( crldist < maxdist )
		oncrl += maxdist - crldist;
	    const int zdist = mNINT32( fabs(crds_[idx].z-crds_[idy].z) /
			              fabs(SI().zStep()) );
	    if ( zdist < maxdist )
		ontms += maxdist - zdist;
	}
    }

    if ( ontms==oninl && ontms==oncrl )
	return Coord3::udf();
    if ( ontms>=oncrl && ontms>=oninl )
	return Coord3( 0, 0, 1 );
    if ( oninl == oncrl )
	return Coord3( Coord::udf(), 0 );

    return oncrl>oninl ? Coord3( SI().binID2Coord().colDir(), 0 ) :
			 Coord3( SI().binID2Coord().rowDir(), 0 );
}


bool FSStoFault3DConverter::FaultStick::pickedOnInl() const
{
    return pickedonplane_ && normal_.isDefined() &&
	   fabs(Coord(normal_).dot(SI().binID2Coord().rowDir()))>0.5;
}


bool FSStoFault3DConverter::FaultStick::pickedOnCrl() const
{
    return pickedonplane_ && normal_.isDefined() &&
	   fabs(Coord(normal_).dot(SI().binID2Coord().colDir()))>0.5;
}


bool FSStoFault3DConverter::FaultStick::pickedOnTimeSlice() const
{
    return pickedonplane_ && normal_.isDefined() && fabs(normal_.z)>0.5;
}


bool FSStoFault3DConverter::FaultStick::pickedOnHorizon() const
{
    return !pickedonplane_ && normal_.isDefined() && fabs(normal_.z)>0.5;
}


FSStoFault3DConverter::Setup::Setup()
    : pickplanedir_(Setup::Auto)
    , sortsticks_(true)
    , zscale_(SI().zScale())
    , stickslopethres_(mUdf(double))
    , useinlcrlslopesep_(false)
    , stickseldir_(Setup::Auto)
    , addtohistory_(false)
{
}

FSStoFault3DConverter::FSStoFault3DConverter( const Setup& setup,
					      const FaultStickSet& fss,
					      Fault3D& f3d )
    : setup_(setup)
    , fss_(fss)
    , fault3d_(f3d)
    , curfssg_(0)
{
}


bool FSStoFault3DConverter::convert()
{
    fault3d_.geometry().selectAllSticks();
    fault3d_.geometry().removeSelectedSticks( setup_.addtohistory_ );
    bool selhorpicked = false;
    for ( int sidx=0; sidx<fss_.nrSections(); sidx++ )
    {
	const EM::SectionID sid = fss_.sectionID( sidx );
	readSection( sid );
	if ( sidx==0 )
	    selhorpicked = preferHorPicked();

	selectSticks( selhorpicked );		
	if ( setup_.sortsticks_ )
	    geometricSort( selhorpicked ? MAXDOUBLE : 0.0 );

	untwistSticks( setup_.zscale_ );
	resolveUdfNormals();
	writeSection( sid );
	deepErase( sticks_ );
    }

    return true;
}


#define mAddStickPoese() \
    for ( int k=0; k<=lastidx; k++ ) \
	sticks_[idy]->crds_ += stickposes[k]; \
    found = true; \
    break; \


bool FSStoFault3DConverter::readSection( const SectionID& sid )
{
    if ( fss_.sectionIndex(sid) < 0 )
	return false;
    mDynamicCast( const Geometry::FaultStickSet*, curfssg_,
		  fss_.sectionGeometry(sid) );
    if ( !curfssg_ )
	return false;
    const StepInterval<int> rowrg = curfssg_->rowRange();
    if ( rowrg.isUdf() )
	return false;

    const float inld = SI().inlDistance() * SI().inlStep();
    const float crld = SI().crlDistance() * SI().crlStep();
    const float zd = SI().zStep() * SI().zScale();
    const float epsilon = Math::Sqrt(inld*inld+crld*crld+zd*zd)/2;
    const float zepsilon = SI().zStep()/2;

    TypeSet<Coord3> singles;
    TypeSet<char> pickedplane;
    TypeSet<int> inlcrl;
    TypeSet<float> zs;
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	const StepInterval<int> colrg = curfssg_->colRange( row );
	if ( colrg.isUdf() )
	    return false;
	
	TypeSet<Coord3> stickposes;
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    const Coord3 pos = curfssg_->getKnot( RowCol(row,col) );
	    if ( pos.isDefined() )
		stickposes += pos;
	}

	const int lastidx = stickposes.size()-1;
	if ( lastidx<1 )
	{
	    if ( !lastidx )
		singles += stickposes[0];
	    continue;
	}
	
	Interval<int> inlrg, crlrg;
	Interval<double> zrg;
	for ( int idy=0; idy<=lastidx; idy++ )
	{
	    const BinID bid = SI().transform( stickposes[idy] );
	    if ( !idy )
	    {
		inlrg.start = inlrg.stop = bid.inl;
		crlrg.start = crlrg.stop = bid.crl;
		zrg.start = zrg.stop = stickposes[idy].z;
	    }
	    else
	    {
		inlrg.include( bid.inl );
		crlrg.include( bid.crl );
		zrg.include( stickposes[idy].z );
	    }
	}

	/*Before making a new stick, check to see if current end points are on
	  any existing stick or not, if so, merge them. */
	bool found = false;
	if ( inlrg.start==inlrg.stop ) 
	{
	    for ( int idy=0; idy<sticks_.size(); idy++ )
	    {
		if ( pickedplane[idy]==mOnInline && inlcrl[idy]==inlrg.start )
		{
		    mAddStickPoese();
		}
	    }
	}
	else if ( crlrg.start==crlrg.stop )
	{
	    for ( int idy=0; idy<sticks_.size(); idy++ )
	    {
		if ( pickedplane[idy]==mOnCrlline && inlcrl[idy]==crlrg.start )
		{
		    mAddStickPoese();
		}
	    }
	}
	else if ( mIsEqual(zrg.start,zrg.stop,zepsilon) )
	{
	    for ( int idy=0; idy<sticks_.size(); idy++ )
	    {
		if ( pickedplane[idy]==mOnZSlice && 
		     mIsEqual(zs[idy],zrg.stop,zepsilon) )
		{
		    mAddStickPoese();
		}
	    }
	}
	else
	{
	    for ( int idy=0; idy<sticks_.size(); idy++ )
	    {
		if ( found ) 
		    break;

    		const int picksz = sticks_[idy]->crds_.size();
		for ( int idz=0; idz<picksz-1; idz++ )
		{
		    Coord3 k0 = sticks_[idy]->crds_[idz]; 
		    k0.z *= SI().zScale();
		    Coord3 k1 = sticks_[idy]->crds_[idz+1];
		    k1.z *= SI().zScale();
		    Line3 segment( k0, k1-k0 );
		    Coord3 tmp = stickposes[0]; tmp.z *= SI().zScale();
		    float dist = (float) segment.distanceToPoint(tmp);
		    if ( dist>epsilon )
		    {
			tmp = stickposes[lastidx]; tmp.z *= SI().zScale();
			dist = (float) segment.sqDistanceToPoint( tmp );
		    }

		    if ( dist<epsilon )
		    {
			mAddStickPoese();
		    }
		}
	    }
	}

	if ( found )
	    continue;

	FaultStick* stick = new FaultStick( row );
	stick->crds_ = stickposes;
	stick->pickedonplane_ = fss_.geometry().pickedOnPlane( sid, row );
	if ( stick->pickedonplane_ )
	    stick->normal_ = stick->findPlaneNormal();
	else
	    stick->normal_ = curfssg_->getEditPlaneNormal( row );
	
	sticks_ += stick;
	pickedplane += (inlrg.start==inlrg.stop ? mOnInline : 
		(crlrg.start==crlrg.stop ? mOnCrlline : 
		(mIsEqual(zrg.start,zrg.stop,zepsilon) ? mOnZSlice:mOnOther)));
	inlcrl += (inlrg.start==inlrg.stop ? inlrg.start : 
		(crlrg.start==crlrg.stop ? crlrg.start : -1));
	zs += (mIsEqual(zrg.start,zrg.stop,zepsilon) ? (float) zrg.start : 0);
    }

    /*Merge single pick to nearest stick, not sorted yet*/
    const int sz = sticks_.size();
    for ( int idx=0; idx<singles.size(); idx++ )
    {
	const Coord3& pos = singles[idx];
	const BinID bid = SI().transform( pos );

	int nearidx = -1;
	for ( int idy=0; idy<sz; idy++ )
	{
	    if ( (pickedplane[idy]==mOnInline && inlcrl[idy]==bid.inl) ||
	         (pickedplane[idy]==mOnCrlline && inlcrl[idy]==bid.crl) ||
	         (pickedplane[idy]==mOnZSlice && 
		  mIsEqual(pos.z,zs[idy],zepsilon)) )
	    {
		nearidx = idy;
		sticks_[idy]->crds_ += pos;
		break;
	    }
	}

	if ( nearidx>-1 )
	    continue;

	float mindist = 0;
	for ( int idy=0; idy<sz; idy++ )
	{
	    const int picksz = sticks_[idy]->crds_.size();
	    for ( int idz=0; idz<picksz-1; idz++ )
	    {
		const Coord3& k0 = sticks_[idy]->crds_[idz];
		const Coord3& k1 = sticks_[idy]->crds_[idz+1];
		Line3 segment( k0, k1-k0 );
		const float pldist = (float) segment.sqDistanceToPoint( pos );
		if ( nearidx==-1 || pldist<mindist )
		{
		    nearidx = idy;
		    mindist = pldist;
		}
	    }
	}
	sticks_[nearidx]->crds_ += pos;
    }

    return true;
}


bool FSStoFault3DConverter::preferHorPicked() const
{
    if ( setup_.pickplanedir_ == Setup::Horizontal )
	return true;
    if ( setup_.pickplanedir_ == Setup::Vertical )
	return false;

    int nrhorpicked = 0;
    for ( int idx=0; idx<sticks_.size(); idx++ )
    {
	if ( sticks_[idx]->pickedOnTimeSlice() ||
	     sticks_[idx]->pickedOnHorizon() )
	    nrhorpicked++;
    }

    return nrhorpicked > sticks_.size()/2;
}


#define insertSorted( slope, slopelist ) \
{ \
    if ( slopelist.isEmpty() || slope >= slopelist[slopelist.size()-1 ] ) \
	slopelist += slope; \
    else \
	for ( int slopeidx=0; slopeidx<slopelist.size(); slopeidx++ ) \
	{ \
	    if ( slope <= slopelist[slopeidx] ) \
	    { \
		slopelist.insert( slopeidx, slope ); \
		break; \
	    } \
	} \
}\


void FSStoFault3DConverter::selectSticks( bool selhorpicked )
{
    for ( int idx=sticks_.size()-1; idx>=0; idx-- )
    {
	bool ishorpicked = sticks_[idx]->pickedOnTimeSlice() ||
	    		   sticks_[idx]->pickedOnHorizon();

	if ( ishorpicked != selhorpicked )
	    delete sticks_.removeSingle( idx );
    }
    if ( selhorpicked )
	return;

    bool useinlcrlsep = setup_.useinlcrlslopesep_;
    double slopethres = setup_.stickslopethres_;
    bool inlsteeper = true;

    if ( useinlcrlsep )
    {
	TypeSet<double> inlslopes;
	TypeSet<double> crlslopes;
	for ( int idx=0; idx<sticks_.size(); idx++ )
	{
	    const double slope = sticks_[idx]->slope( setup_.zscale_ );
	    if ( !mIsUdf(slope) && sticks_[idx]->pickedOnInl() )
		insertSorted( slope, inlslopes ); 
	    if ( !mIsUdf(slope) && sticks_[idx]->pickedOnCrl() )
		insertSorted( slope, crlslopes ); 
	}
	const int inlsz = inlslopes.size();
	const int crlsz = crlslopes.size();

	if ( inlslopes.isEmpty() || crlslopes.isEmpty() )
	    useinlcrlsep = false;
	else
	{
	    inlsteeper = inlslopes[ inlsz/2 ] > crlslopes[ crlsz/2 ];

	    if ( mIsUdf(slopethres) )
	    {
		slopethres = inlsteeper ? (inlslopes[0]+crlslopes[crlsz-1])/2 :
					  (crlslopes[0]+inlslopes[inlsz-1])/2; 
	    }
	}
    }

    if ( mIsUdf(slopethres) )
	return;

    bool selhorsticks = setup_.stickseldir_==Setup::Horizontal;
    if ( setup_.stickseldir_ == Setup::Auto )
    {
	int horvertbalance = 0;
	for ( int idx=0; idx<sticks_.size(); idx++ )
	{
	    const double slope = sticks_[idx]->slope( setup_.zscale_ );
	    if ( !mIsUdf(slope) )
		horvertbalance += slope<fabs(slopethres) ? -1 : 1;
	}
	if ( horvertbalance < 0 )
	    selhorsticks = true;
    }		

    for ( int idx=sticks_.size()-1; idx>=0; idx-- )
    {
	const FaultStick& stick = *sticks_[idx];
	if ( useinlcrlsep && (stick.pickedOnInl() || stick.pickedOnCrl()) )
	{
	    if ( (stick.pickedOnInl() && inlsteeper==selhorsticks) ||
		 (stick.pickedOnCrl() && inlsteeper!=selhorsticks) )
	    {
		delete sticks_.removeSingle( idx );
		continue;
	    }
	    if ( mIsUdf(setup_.stickslopethres_) )
		continue;
	}

	const double slope = stick.slope( setup_.zscale_ );
	if ( !mIsUdf(slope) && (slope<fabs(slopethres)) != selhorsticks )
	    delete sticks_.removeSingle( idx );
    }
}


void FSStoFault3DConverter::geometricSort( double zscale )
{
    TypeSet<int> sticknrs;
    for ( int idx=0; idx<sticks_.size(); idx++ )
	sticknrs += sticks_[idx]->sticknr_;

    if ( curfssg_ )
	curfssg_->geometricStickOrder( sticknrs, zscale, false );

    for ( int idy=sticknrs.size()-1; idy>0; idy-- )
    {
	for ( int idx=0; idx<sticks_.size(); idx++ )
	{
	    if ( sticks_[idx]->sticknr_ == sticknrs[idy] )
	    {
		sticks_.swap( idx, idy );
		break;
	    }
	}
    }

    if ( preferHorPicked() )
	return;

    for ( int idx=0; idx<sticks_.size(); idx++ )
    {
	const int nrcrds = sticks_[idx]->crds_.size();
	if ( nrcrds<2 ) continue;

	TypeSet<int> tmp;
	TypeSet<float> zs;
	for ( int idy=0; idy<nrcrds; idy++ )
	{
	    zs += (float) sticks_[idx]->crds_[idy].z;
	    tmp += idy;
	}
	
	sort_coupled( zs.arr(), tmp.arr(), nrcrds );
	const bool ascend = zs[0]<zs[nrcrds-1];
	
	TypeSet<Coord3> newcrds;
	if ( ascend )
	{
	    for ( int idy=0; idy<nrcrds; idy++ )
		newcrds += sticks_[idx]->crds_[tmp[idy]];
	}
	else
	{
	    for ( int idy=nrcrds-1; idy>=0; idy-- )
		newcrds += sticks_[idx]->crds_[tmp[idy]];
	}
	sticks_[idx]->crds_ = newcrds;
    }
}


void FSStoFault3DConverter::untwistSticks( double zscale )
{
    for ( int idx=1; idx<sticks_.size(); idx++ )
    {
	if ( !curfssg_ || !curfssg_->isTwisted(sticks_[idx-1]->sticknr_,
		    sticks_[idx]->sticknr_, zscale) )
	    continue;

	const int nbnrknots = sticks_[idx-1]->crds_.size();
	const int nrknots = sticks_[idx]->crds_.size();
	if ( nbnrknots<2 || nrknots<2 )
	    continue;

	Coord3 d0 = sticks_[idx-1]->crds_[0]-sticks_[idx-1]->crds_[nbnrknots-1];
	d0.z *= zscale;
	Coord3 d1 = sticks_[idx]->crds_[0]-sticks_[idx]->crds_[nrknots-1];
	d1.z *= zscale;
	double cosangle = d0.dot(d1)/Math::Sqrt(d0.dot(d0)*d1.dot(d1));
	if ( fabs(cosangle)<0.707 ) //if skewed more than 45 degree ignore it
	    continue;

	for ( int idy=0; idy<nrknots/2; idy++ )
	    sticks_[idx]->crds_.swap( idy, nrknots-1-idy );
    }
}


void FSStoFault3DConverter::resolveUdfNormals()
{
    for ( int idx=0; idx<sticks_.size(); idx++ )
    {
	Coord3& normal = sticks_[idx]->normal_;
	if ( normal.isDefined() )
	    continue;

	for ( int idy=(idx ? idx-1 : idx+1); idy<sticks_.size(); idy++ )
	{
	    Coord3& adjacentnormal = sticks_[idy]->normal_;
	    if ( !adjacentnormal.isDefined() )
		continue;
	    if ( mIsUdf(normal.z) || normal.z==adjacentnormal.z )
	    {
		normal = adjacentnormal;
		break;
	    }
	}
	if ( !normal.isDefined() )
	    normal = Coord3( SI().binID2Coord().rowDir(), 0 );
    }	
}


bool FSStoFault3DConverter::writeSection( const SectionID& sid ) const
{
    if ( sticks_.isEmpty() )
	return false;

    if ( fault3d_.sectionIndex(sid) )
	fault3d_.geometry().addSection( fss_.sectionName(sid), sid, false );

    int sticknr = sticks_[0]->sticknr_;
    for ( int idx=1; idx<sticks_.size(); idx++ )
    {
	if ( sticks_[idx]->sticknr_ < sticknr )
	    sticknr = sticks_[idx]->sticknr_;
    }

    for ( int idx=0; idx<sticks_.size(); idx++ )
    {
	const FaultStick* stick = sticks_[idx];
	if ( stick->crds_.isEmpty() )
  	    continue;

	if ( !fault3d_.geometry().insertStick( sid, sticknr, 0,
			stick->crds_[0], stick->normal_, setup_.addtohistory_) )
	    continue;

	for ( int crdidx=1; crdidx<stick->crds_.size(); crdidx++ )
	{
	    const RowCol rc( sticknr, crdidx );
	    fault3d_.geometry().insertKnot( sid, rc.toInt64(),
				stick->crds_[crdidx], setup_.addtohistory_ );
	}

	sticknr++;
    }

    return fault3d_.geometry().nrSticks( sid );
}



}; // namespace EM
