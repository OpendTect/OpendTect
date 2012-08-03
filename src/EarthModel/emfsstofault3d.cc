/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: emfsstofault3d.cc,v 1.15 2012-08-03 21:34:08 cvsyuancheng Exp $";

#include "emfsstofault3d.h"

#include "survinfo.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "sorting.h"


namespace  EM
{


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
    bool selhorpicked;

    for ( int sidx=0; sidx<fss_.nrSections(); sidx++ )
    {
	const int sid = fss_.sectionID( sidx );
	readSection( sid );

	if ( !sidx )
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

    RowCol rc;
    for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
    {
	const StepInterval<int> colrg = curfssg_->colRange( rc.row );
	if ( colrg.isUdf() )
	    return false;

	FaultStick* stick = new FaultStick( rc.row );
	sticks_ += stick;

	for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	{
	    const Coord3 pos = curfssg_->getKnot( rc );
	    if ( !pos.isDefined() )
		return false;
	    stick->crds_ += pos;
	}

	stick->pickedonplane_ = fss_.geometry().pickedOnPlane( sid, rc.row );
	if ( stick->pickedonplane_ )
	    stick->normal_ = stick->findPlaneNormal();
	else
	    stick->normal_ = curfssg_->getEditPlaneNormal( rc.row );
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
	    delete sticks_.remove( idx );
    }
    if ( selhorpicked )
	return;

    bool useinlcrlsep = setup_.useinlcrlslopesep_;
    double slopethres = setup_.stickslopethres_;
    bool inlsteeper;

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
		delete sticks_.remove( idx );
		continue;
	    }
	    if ( mIsUdf(setup_.stickslopethres_) )
		continue;
	}

	const double slope = stick.slope( setup_.zscale_ );
	if ( !mIsUdf(slope) && (slope<fabs(slopethres)) != selhorsticks )
	    delete sticks_.remove( idx );
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

    for ( int idx=0; idx<sticks_.size(); idx++ )
    {
	const int nrcrds = sticks_[idx]->crds_.size();
	if ( nrcrds<2 ) continue;

	TypeSet<int> tmp;
	TypeSet<float> zs;
	for ( int idy=0; idy<nrcrds; idy++ )
	{
	    zs += sticks_[idx]->crds_[idy].z;
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
    bool reverse = false;
    int refidx = 0;
    for ( int idx=1; idx<sticks_.size(); idx++ )
    {
	if ( curfssg_ && curfssg_->isTwisted(sticks_[refidx]->sticknr_,
					     sticks_[idx]->sticknr_, zscale) )
	    reverse = !reverse;
	if ( !reverse ) continue;

	const int nrknots = sticks_[idx]->crds_.size();
	if ( nrknots > 1 )
	    refidx = idx;
	else
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
