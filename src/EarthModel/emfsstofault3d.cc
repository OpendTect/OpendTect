/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        J.C. Glas
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: emfsstofault3d.cc,v 1.1 2009-04-06 12:52:22 cvsjaap Exp $";

#include "emfsstofault3d.h"

#include "survinfo.h"
#include "emfaultstickset.h"
#include "emfault3d.h"


namespace  EM
{


FSStoFault3DConverter::FaultStick::FaultStick( int sticknr )
	: sticknr_(sticknr)
	, pickedonplane_(false)
	, normal_(Coord3::udf())
{}


double FSStoFault3DConverter::FaultStick::distTo( const FaultStick& fs,
						  double zscale,
						  bool* istwisted ) const
{
/*  Take distance between the two end points of each stick. Difference
    of both distances is minimum amount of rope required to connect both
    sticks_ when moving freely. Our distance measure equals the amount of
    extra rope required when both sticks_ are fixed in space. */
   
    if ( crds_.isEmpty() || fs.crds_.isEmpty() )
	return mUdf(double);
   
    Coord3 a0 = crds_[0];
    Coord3 a1 = crds_[crds_.size()-1];
    Coord3 b0 = fs.crds_[0];
    Coord3 b1 = fs.crds_[fs.crds_.size()-1];

    if ( zscale==MAXDOUBLE )
    {
	a0.x=0; a0.y=0; a1.x=0; a1.y=0; b0.x=0; b0.y=0; b1.x=0; b1.y=0; 
    }
    else 
    {
	a0.z *= zscale; a1.z *= zscale; b0.z *= zscale; b1.z *= zscale;
    }

    const double mindist  = fabs( a0.distTo(a1)-b0.distTo(b1) );
    const double straight = a0.distTo(b0) + a1.distTo(b1) - mindist;
    const double crossed  = a0.distTo(b1) + a1.distTo(b0) - mindist;

    if ( istwisted )
	*istwisted = straight > crossed;

    return mMIN( straight, crossed );
}


void FSStoFault3DConverter::FaultStick::untwist( const FaultStick& fs,
						 double zscale )
{
    bool istwisted;
    distTo( fs, zscale, &istwisted );
    if ( istwisted )
    {
	for ( int idx=0; idx<crds_.size()/2; idx++ )
	    crds_.swap( idx, crds_.size()-1-idx );
    }
}


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
    int oninl = 0; int oncrl = 0; int ontms = 0;

    for ( int idx=0; idx<crds_.size()-1; idx++ )
    {
	const BinID bid0 = SI().transform( crds_[idx] );
	for ( int idy=idx+1; idy<crds_.size(); idy++ )
	{
	    const BinID bid1 = SI().transform( crds_[idy] );
	    if ( bid0.inl == bid1.inl )
		oninl++;
	    if ( bid0.crl == bid1.crl )
		oncrl++;
	    if ( fabs(crds_[idx].z-crds_[idy].z) < fabs(0.5*SI().zStep()) )
		ontms++;
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


FSStoFault3DConverter::Setup::Setup()
    : pickplanedir_(Setup::Auto)
    , sortsticks_(true)
    , zscale_(SI().zScale())
    , stickslopethres_(mUdf(double))
    , useinlcrlslopesep_(false)
    , stickseldir_(Setup::Auto)
{
}

FSStoFault3DConverter::FSStoFault3DConverter( const Setup& setup,
					      const FaultStickSet& fss,
					      Fault3D& f3d )
    : setup_(setup)
    , fss_(fss)
    , fault3d_(f3d)
{
}


bool FSStoFault3DConverter::convert()
{
    fault3d_.removeAll();
    bool selhorpicked;

    for ( int sidx=0; sidx<fss_.nrSections(); sidx++ )
    {
	const int sid = fss_.sectionID( sidx );
	readSection( sid );

	if ( !sidx )
	    selhorpicked = preferHorPicked();

	selectSticks( selhorpicked );
		
	if ( setup_.sortsticks_ )
	    geometricSort( selhorpicked ? MAXDOUBLE : 0.0  );

	for ( int idx=1; idx<sticks_.size(); idx++ )
	    sticks_[idx]->untwist( *sticks_[idx-1], setup_.zscale_ );

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
    mDynamicCastGet( const Geometry::FaultStickSet*, fssg,
		     fss_.sectionGeometry(sid) );
    if ( !fssg )
	return false;
    const StepInterval<int> rowrg = fssg->rowRange();
    if ( rowrg.isUdf() )
	return false;

    RowCol rc;
    for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
    {
	const StepInterval<int> colrg = fssg->colRange( rc.row );
	if ( colrg.isUdf() )
	    return false;

	FaultStick* stick = new FaultStick( rc.row );
	sticks_ += stick;

	for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	{
	    const Coord3 pos = fssg->getKnot( rc );
	    if ( !pos.isDefined() )
		return false;
	    stick->crds_ += pos;
	}

	stick->pickedonplane_ = fss_.geometry().pickedOnPlane( sid, rc.row );
	if ( stick->pickedonplane_ )
	    stick->normal_ = stick->findPlaneNormal();
	else
	    stick->normal_ = fssg->getEditPlaneNormal( rc.row );
    }
    return true;
}


bool FSStoFault3DConverter::preferHorPicked() const
{
    if ( setup_.pickplanedir_ == Setup::Horizontal )
	return true;
    if ( setup_.pickplanedir_ == Setup::Vertical )
	return false;

    int nrpickedontimeslice = 0;
    for ( int idx=0; idx<sticks_.size(); idx++ )
    {
	if ( sticks_[idx]->pickedOnTimeSlice() )
	    nrpickedontimeslice++;
    }

    return nrpickedontimeslice > sticks_.size()/2;
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
	if ( sticks_[idx]->pickedOnTimeSlice() != selhorpicked )
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
    if ( sticks_.size()>2 )
    {
	double mindist = MAXDOUBLE;
	int minidx0, minidx1;

	for ( int idx=0; idx<sticks_.size()-1; idx++ )
	{
	    for ( int idy=idx+1; idy<sticks_.size(); idy++ )
	    {
		const double dist =
			     sticks_[idx]->distTo( *sticks_[idy], zscale );

		if ( dist < mindist )
		{
		    mindist = dist;
		    minidx0 = idx;
		    minidx1 = idy;
		}
	    }
	}
	sticks_.swap( 0, minidx0 );
	sticks_.swap( 1, minidx1 );

	for ( int tailidx=1; tailidx<sticks_.size()-1; tailidx++ )
	{
	    mindist = MAXDOUBLE;
	    bool reverse = false;
	    for ( int idy=tailidx+1; idy<sticks_.size(); idy++ )
	    {
		const double dist0 =
			     sticks_[0]->distTo( *sticks_[idy], zscale );
		const double dist1 =
			     sticks_[tailidx]->distTo( *sticks_[idy], zscale );

		if ( mMIN(dist0,dist1) < mindist )
		{
		    mindist = mMIN( dist0, dist1 );
		    minidx0 = idy;
		    reverse = dist0 < dist1;
		}
	    }
	    for ( int idx=0; reverse && idx<tailidx*0.5; idx++ )
		sticks_.swap( idx, tailidx-idx );

	    sticks_.swap( tailidx+1, minidx0 );
	}
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

	if ( !fault3d_.geometry().insertStick(sid, sticknr, 0, stick->crds_[0],
		    			      stick->normal_, false) )
	    continue;

	for ( int crdidx=1; crdidx<stick->crds_.size(); crdidx++ )
	{
	    const RowCol rc( sticknr, crdidx );
	    fault3d_.geometry().insertKnot( sid, rc.getSerialized(),
					    stick->crds_[crdidx], false );
	}

	sticknr++;
    }

    return fault3d_.geometry().nrSticks( sid );
}



}; // namespace EM
