/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2009
-*/


#include "contcurvinterpol.h"
#include "arrayndimpl.h"
#include "math2.h"
#include <math.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include "limits.h"

#define mMaxIterations 250
#define mPadSize 2

static const double cLepsilon = 1.0;
static const double cE2 = cLepsilon*cLepsilon;
static const double cEpsP2 = cE2;
static const double cEpsM2 = 1.0/cE2;
static const double cOneplusE2 = 1.0 + cE2;
static const double cTwopluseP2 = 2.0 + 2.0*cEpsP2;
static const double cTwoplusM2 = 2.0 + 2.0*cEpsM2;

const char* ContinuousCurvatureArray2DInterpol::sKeyTension()
{
    return "Tension";
}

const char* ContinuousCurvatureArray2DInterpol::sKeySearchRadius()
{
    return "Search Radius";
}

class HorizonDataComparer
{
public:
    HorizonDataComparer( const ContinuousCurvatureArray2DInterpol* interpol,
	int gridsize )
	: interpol_(interpol)
	, gridsize_( gridsize )
    {}

    bool operator() ( const ContinuousCurvatureArray2DInterpol::HorizonData&
	dataa, const ContinuousCurvatureArray2DInterpol::HorizonData& datab )
    {
	const int idx1 = dataa.index_;
	const int idx2 = datab.index_;

	if ( idx1<idx2 ) return true;
	if ( idx1>idx2 ) return false;

	const int blockny = ( interpol_->nrcols_-1 )/gridsize_ + 1;

	const double x0 = (double)(idx1/blockny) * gridsize_;
	const double y0 = (double)(idx1%blockny) * gridsize_;

	const double dist1=
	    (dataa.x_-x0)*(dataa.x_-x0)+(dataa.y_-y0)*(dataa.y_-y0);
	const double dist2=
	    (datab.x_-x0)*(datab.x_-x0)+(datab.y_-y0)*(datab.y_-y0);

	if ( dist1<dist2 )
	    return true;
	if ( dist1>dist2 )
	    return false;

	return false;
    }

protected:
    const ContinuousCurvatureArray2DInterpol* interpol_;
    const int gridsize_;

};


class GridInitializer: public ParallelTask
{
public:
GridInitializer( ContinuousCurvatureArray2DInterpol* p, int gridsize,
		 od_int64 size)
    : interpol_( p )
    , nriterations_( size )
    , gridsize_(gridsize)
    , blocknx_( 0 )
    , blockny_( 0 )
    , irad_( 0 )
    , jrad_( 0 )
    , rfact_( 0 )
{}

protected:
    bool doPrepare( int )
    {
	if ( gridsize_<1 || interpol_->radius_<=0 )
	    return false;

	blockny_ = ( interpol_->nrcols_-1 )/gridsize_ + 1;
	irad_ = (int)( Math::Ceil(interpol_->radius_/gridsize_) );
	jrad_ = (int)( Math::Ceil(interpol_->radius_/gridsize_) );
	rfact_ = -4.5/( interpol_->radius_*interpol_->radius_ );

	return true;
    }

    bool doWork( od_int64 start, od_int64 stop, int )
    {
	for ( int row = 0; row<(int)stop; row++ )
	{
	    if ( row>= blocknx_ ) break;
	    double x0 = row*gridsize_;
	    for ( int col = 0; col<blockny_; col++)
	    {
		const double y0 = col*gridsize_;
		int imin = row - irad_;
		if ( imin<0 ) imin = 0;
		int imax = row + irad_;
		if ( imax >= blocknx_ ) imax = blocknx_ - 1;
		int jmin = col - jrad_;
		if ( jmin<0 ) jmin = 0;
		int jmax = col + jrad_;
		if ( jmax>=blockny_ ) jmax = blockny_ - 1;
		const int index1 = imin*blockny_ + jmin;
		const int index2 = imax*blockny_ + jmax + 1;

		double sumw = 0;
		double sumzw = 0.0;
		int k = 0;

		while ( k<interpol_->nrdata_ &&
		    interpol_->hordata_[k].index_<index1 )
		    k++;

		for ( int ki = imin; k<interpol_->nrdata_ &&
		    ki<=imax && interpol_->hordata_[k].index_<index2; ki++ )
		{
		    for( int kj = jmin; k<interpol_->nrdata_ &&
			kj<=jmax && interpol_->hordata_[k].index_<index2; kj++ )
		    {
			const int kindex = ki*blockny_ + kj;
			while ( k<interpol_->nrdata_ &&
			    interpol_->hordata_[k].index_<kindex )
			    k++;

			while ( k<interpol_->nrdata_ &&
			    interpol_->hordata_[k].index_==kindex )
			{
			    const double r = ( interpol_->hordata_[k].x_-x0 )*
				( interpol_->hordata_[k].x_-x0 ) +
				( interpol_->hordata_[k].y_-y0 ) *
				( interpol_->hordata_[k].y_-y0 );
			    const double weight = Math::Exp( rfact_*r );
			    sumw += weight;
			    sumzw += weight*interpol_->hordata_[k].z_;
			    k++;
			}
		    }
		}

		const int padnrcols = interpol_->nrcols_ + 2*mPadSize;
		const int rcswcorner = 2*padnrcols+2;
		const int blkidx = ( row*padnrcols+col )*gridsize_;
		if ( sumw==0.0 )
		    interpol_->griddata_[rcswcorner+blkidx] =
		    (float)interpol_->zmean_;
		else
		    interpol_->griddata_[rcswcorner+blkidx] =
		    (float)( sumzw/sumw );
	    }
	}
	return true;
    }

    od_int64	nrIterations() const { return nriterations_; }

private:
    ContinuousCurvatureArray2DInterpol* interpol_;
    od_int64 nriterations_;
    int blocknx_;
    int blockny_;
    int gridsize_;
    int irad_;
    int jrad_;
    double rfact_;
};


ContinuousCurvatureArray2DInterpol::ContinuousCurvatureArray2DInterpol()
    : curdefined_( 0 )
    , nodestofill_( 0 )
    , totalnr_( mMaxIterations )
    , radius_( .0f )
    , convergelimit_( .0f )
    , tension_( .25f )
    , zmean_( .0f )
    , nfact_( 0 )
    , zscale_( .0f )
    , planec0_( .0f )
    , planec1_( .0f )
    , planec2_( .0f )
    , nrdata_( 0 )
{
    setName( "Grid interpolating..." );
}


ContinuousCurvatureArray2DInterpol::~ContinuousCurvatureArray2DInterpol()
{
    delete [] curdefined_;
    delete [] nodestofill_;

    gridstatus_.erase();
    griddata_.erase();
    hordata_.erase();
}


bool ContinuousCurvatureArray2DInterpol::setArray( Array2D<float>& arr,
    TaskRunner* taskrunner )
{
    if ( !Array2DInterpol::setArray(arr,taskrunner) )
	return false;

    return initFromArray( taskrunner );
}


bool ContinuousCurvatureArray2DInterpol::setArray( ArrayAccess& arr,
    TaskRunner* taskrunner )
{
    if ( !canUseArrayAccess() )
	return false;

    arr_ = 0;
    arrsetter_ = &arr;
    nrrows_ = arr.getSize(0);
    nrcols_ = arr.getSize(1);
    nrcells_ = nrrows_*nrcols_;

    return initFromArray( taskrunner );

}


bool ContinuousCurvatureArray2DInterpol::initFromArray(
    TaskRunner* taskrunner )
{
    if ( !arr_ && !arrsetter_ )
	return false;

    delete [] curdefined_;
    mTryAlloc( curdefined_, bool[nrcells_] );
    if ( !curdefined_ )
	return false;

     delete [] nodestofill_;
    mTryAlloc( nodestofill_, bool[nrcells_] );
    if ( !nodestofill_ )
	return false;

    nrdata_ = 0;
    for ( int idx = 0; idx<nrcells_; idx++ )
    {
	curdefined_[idx] = isDefined( idx );
	if ( curdefined_[idx] )
	    nrdata_++;
    }

    if ( !fillInputData() )
	return false;

    getNodesToFill( curdefined_, nodestofill_, taskrunner );

    return true;
}


bool ContinuousCurvatureArray2DInterpol::fillInputData()
{
    if ( nrdata_== 0 ) return false;
    mTryAlloc( hordata_, HorizonData[nrdata_] );

    float zmean = .0;
    int count = 0;

    for ( int idx = 0; idx<nrcells_; idx++ )
    {
	curdefined_[idx] = isDefined( idx );
	if ( curdefined_[idx] )
	{
	    const int i = mNINT32( Math::Floor(( idx/nrcols_+0.5 )) );
	    const int j = mNINT32( Math::Floor(( idx%nrcols_+0.5 )) );
	    HorizonData hd;
	    hd.index_ = i*nrcols_ + j;
	    hd.x_ = (float)i;
	    hd.y_ = (float)j;
	    hd.z_ = arr_->getData()[idx];
	    hordata_[count] = hd;
	    zmean += hd.z_;
	    count++;
	}
    }

    if ( count>0 )
	zmean_ = zmean/count;

    return true;
}

#define mConstBlockSize()\
const int grideast = gridsize*padnrcols;\
const int blocknx = (nrrows_-1)/gridsize + 1;\
const int blockny = (nrcols_-1)/gridsize + 1;\

#define mConstCorners()\
const int rcswcorner = 2*padnrcols + 2;\
const int rcsecorner = rcswcorner + ( nrrows_-1 )*padnrcols;\
const int rcnwcorner = rcswcorner + nrcols_- 1;\
const int rcnecorner = rcsecorner + nrcols_- 1;


bool ContinuousCurvatureArray2DInterpol::doPrepare( int )
{

    if ( tension_>=1.0 )
	return false;

    factors_.setSize( 32 );
    factors_.setAll( 0 );

    mTryAlloc( briggs_, BriggsData[nrdata_] );
    if ( !briggs_ )
	return false;

    const int padcells = (nrrows_+2*mPadSize)*(nrcols_+2*mPadSize);

    mTryAlloc( griddata_, float[padcells] );
    mTryAlloc( gridstatus_, char[padcells] );

    for ( int idx=0; idx<padcells; idx++ )
    {
	griddata_[idx] = 0;
	gridstatus_[idx] = 0;
    }

    if ( !removePlanarTrend() || !rescaleZValues() || !setCoefficients() )
	return false;

    return true;
}


int ContinuousCurvatureArray2DInterpol::verifyGridSize( int gridsize )
{
    int blocknx = (nrrows_-1)/gridsize + 1;
    int blockny = (nrcols_-1)/gridsize + 1;

    while ( blocknx<4 || blockny<4 )
    {
	gridsize = getNextGridSize( gridsize );
	blocknx = (nrrows_-1)/gridsize + 1;
	blockny = (nrcols_-1)/gridsize + 1;
    }

    if ( blocknx<4 || blockny<4 )
	return -1;

    return gridsize;
}


bool ContinuousCurvatureArray2DInterpol::doWork( od_int64, od_int64, int )
{
    const int gridsize = calcPrimeFactors( calcGcdEuclid() );

    int curgridsize = verifyGridSize( gridsize );
    if ( curgridsize<0 ) return false;

    updateEdgeConditions( curgridsize );
    updateGridIndex( curgridsize );

    if ( radius_>0 )
    {
	const int blocknx = ( nrrows_-1 )/curgridsize + 1;
	GridInitializer grdinitializer( this, curgridsize, blocknx );
	grdinitializer.execute();
    }

    findNearestPoint( curgridsize );
    doFiniteDifference( curgridsize );

    int oldgridsize = curgridsize;
    while ( curgridsize>1 )
    {
	curgridsize = getNextGridSize( curgridsize );
	updateGridConditions( curgridsize );

	fillInForecast( oldgridsize,curgridsize );
	doFiniteDifference( curgridsize );
	findNearestPoint( curgridsize );
	doFiniteDifference( curgridsize );
	oldgridsize = curgridsize;
    }

    finalizeGrid();
    recoverPlanarTrend();
    updateArray2D();

    return true;
}


void ContinuousCurvatureArray2DInterpol::updateGridConditions( int gridsize )
{
    updateEdgeConditions( gridsize );
    updateGridIndex( gridsize );
}

int ContinuousCurvatureArray2DInterpol::getNextGridSize( int curgridsize )
{
    const int gridsize = curgridsize/factors_[nfact_-1];
    nfact_--;
    return gridsize;
}


void ContinuousCurvatureArray2DInterpol::recoverPlanarTrend()
{
    const int padnrcols = nrcols_ + 2*mPadSize;
    const int rcswcorner = 2*padnrcols + 2;
    for ( int row=0; row<nrrows_; row++ )
    {
	for ( int col=0; col<nrcols_; col++ )
	{
	    const int rc = (int)( rcswcorner + row*padnrcols + col );
	    griddata_[rc] = (float)( (griddata_[rc]*zscale_) +
		(planec0_+planec1_*row+planec2_*col) );
	}
    }
}


bool ContinuousCurvatureArray2DInterpol::updateArray2D()
{
    const int padnrrows = nrrows_ + 2*mPadSize;
    const int padnrcols = nrcols_ + 2*mPadSize;
    for ( int r=mPadSize; r<padnrrows-mPadSize; r++ )
      {
	  for ( int c=mPadSize; c<padnrcols-mPadSize; c++ )
	  {
	      int idx= (od_uint64)r*padnrcols+c;
	      int odidx = (r-mPadSize)*nrcols_+(c-mPadSize);
	      if ( nodestofill_[odidx] )
		  arr_->set( r-mPadSize, c-mPadSize, griddata_[idx] );
	  }
      }

    return true;
}


void ContinuousCurvatureArray2DInterpol::finalizeGrid()
{
   //Pre-calculate frequently used constants.
    const double  x0const = 4.0*(1.0-tension_)/(2.0-tension_);
    const double  x1const = (3*tension_-2.0)/(2.0-tension_);
    const double  ydenom =  2*cLepsilon*(1.0-tension_)+tension_;
    const double  y0const = 4*cLepsilon*(1.0 -tension_)/ydenom;
    const double  y1const = ( tension_-2*cLepsilon*(1.0-tension_) )/ydenom;
    const int my = nrcols_ + 2*mPadSize;
    const int move[12] = { 2, 1-my, 1, 1+my, -2*my, -my, my,
			   2*my, -1-my, -1, -1+my, -2 };

    const int padnrcols = nrcols_+2*mPadSize;
    mConstCorners();

    for ( int r=0; r<nrrows_; r++ )
    {
	int rc = rcswcorner + r*my;
	griddata_[rc-1] =
	    (float)( y0const*griddata_[rc]+y1const*griddata_[rc+1] );
	rc = rcnwcorner + r*my;
	griddata_[rc+1] =
	    (float)( y0const*griddata_[rc]+y1const*griddata_[rc-1] );
    }

    for ( int c=0; c<nrcols_; c++ )
    {
	int rc = rcswcorner + c;
	griddata_[rc-my] = (float)( x1const *
	    griddata_[rc+my] + x0const * griddata_[rc] );
	rc = rcsecorner + c;
	griddata_[rc+my] = (float)(
	    x1const*griddata_[rc-my] + x0const*griddata_[rc] );
    }

    int rc = rcswcorner;
    griddata_[rc-my-1] =
	griddata_[rc+my-1] + griddata_[rc-my+1] - griddata_[rc+my+1];
    rc = rcnwcorner;
    griddata_[rc-my+1] =
	griddata_[rc+my+1] + griddata_[rc-my-1] - griddata_[rc+my-1];
    rc = rcsecorner;
    griddata_[rc+my-1] =
        griddata_[rc-my-1] + griddata_[rc+my+1] - griddata_[rc-my+ 1];
    rc = rcnecorner;
    griddata_[rc+my+1] =
        griddata_[rc-my+1] + griddata_[rc+my-1] - griddata_[rc-my-1];

    for ( int r=0; r<nrrows_; r++ )
    {
	rc = rcswcorner + r*my;
	griddata_[rc + move[11]] = (float)( griddata_[rc+move[0]] +
	cEpsM2*( griddata_[rc+move[1]] + griddata_[rc+move[3]] -
	- griddata_[rc+move[8]] - griddata_[rc + move[10]] ) +
	cTwoplusM2*( griddata_[rc+move[9]]-griddata_[rc+move[2]]) );

	rc = rcnwcorner + r*my;
	griddata_[rc+move[0]] = -(float)(-griddata_[rc+move[11]] +
	    cEpsM2*(griddata_[rc+move[1]] + griddata_[rc+move[3]] -
	    griddata_[rc+move[8]] - griddata_[rc+move[10]]) +
	    cTwoplusM2*(griddata_[rc+move[9]]-griddata_[rc+move[2]]) );
    }

    for ( int c=0; c<nrcols_; c++ )
    {
	rc = rcswcorner + c;
	griddata_[rc+move[4]] = griddata_[rc+move[7]] +
	    (float)( cEpsP2*(griddata_[rc+move[3]]+griddata_[rc+move[10]]
		-griddata_[rc+move[1]] - griddata_[rc + move[8]]) +
	    cTwopluseP2*(griddata_[rc+move[5]]-griddata_[rc+move[6]]) );

	rc = rcsecorner + c;
	griddata_[rc+move[7]] = -(float)(-griddata_[rc+move[4]] +
	    cEpsP2*( griddata_[rc+move[3]]+griddata_[rc+move[10]] -
		griddata_[rc + move[1]] - griddata_[rc+move[8]]) +
	cTwopluseP2*(griddata_[rc+move[5]]-griddata_[rc+move[6]]) );
    }

}


bool ContinuousCurvatureArray2DInterpol::fillPar( IOPar& par ) const
{
    Array2DInterpol::fillPar( par );
    par.set( sKeyTension(), tension_ );
    par.set( sKeySearchRadius(), radius_ );
    return true;
}


bool ContinuousCurvatureArray2DInterpol::usePar( const IOPar& par )
{
    Array2DInterpol::usePar( par );
    par.get( sKeyTension(), tension_ );
    par.get( sKeySearchRadius(), radius_ );
    return true;
}


bool ContinuousCurvatureArray2DInterpol::removePlanarTrend()
{
    if ( nrdata_ <=0 ) return false;

    double sx(0), sy(0), sz(0), sxx(0), sxy(0), sxz(0), syy(0), syz(0);
    for ( int idx=0; idx<nrdata_; idx++ )
    {
	sx  += hordata_[idx].x_;
	sy  += hordata_[idx].y_;
	sz  += hordata_[idx].z_;
	sxx += hordata_[idx].x_*hordata_[idx].x_;
	sxy += hordata_[idx].x_*hordata_[idx].y_;
	sxz += hordata_[idx].x_*hordata_[idx].z_;
	syy += hordata_[idx].y_*hordata_[idx].y_;
	syz += hordata_[idx].y_*hordata_[idx].z_;
    }

    const od_uint64 count = nrdata_;
    const double d = count*sxx*syy + 2*sx*sy*sxy - count*sxy*sxy -
		     sx*sx*syy - sy*sy*sxx;

    if ( d==0.0 )
    {
	planec0_ = planec1_ = planec2_ = 0.0;
	return true;
    }

    const double a = sz*sxx*syy + sx*sxy*syz + sy*sxy*sxz - sz*sxy*sxy -
		     sx*sxz*syy - sy*syz*sxx;
    const double b = count*sxz*syy + sz*sy*sxy + sy*sx*syz - count*sxy*syz -
		     sz*sx*syy - sy*sy*sxz;
    const double c = count*sxx*syz + sx*sy*sxz + sz*sx*sxy - count*sxy*sxz -
		     sx*sx*syz - sz*sy*sxx;

    planec0_ = a/d;
    planec1_ = b/d;
    planec2_ = c/d;

    for ( int idx=0; idx<nrdata_; idx++ )
    {
	hordata_[idx].z_ -= (float)( planec0_+planec1_*hordata_[idx].x_+
	    planec2_*hordata_[idx].y_ );
    }

    return true;
}

#define GMT_CONV_LIMIT	1.0e-8
bool ContinuousCurvatureArray2DInterpol::rescaleZValues()
{
    double ssz = 0.0;
    int count = 0;

    for ( int idx = 0; idx<nrdata_; idx++ )
    {
	ssz += (double)( hordata_[idx].z_*hordata_[idx].z_ );
	count++;
    }

    zscale_ = Math::Sqrt( ssz/count );

    if ( zscale_<GMT_CONV_LIMIT ) return false;

    for ( int idx=0; idx<nrdata_; idx++ )
	hordata_[idx].z_ /= (float)zscale_;

    if ( convergelimit_ == 0.0 )
	convergelimit_ = 0.001*zscale_;

    return true;

}


int ContinuousCurvatureArray2DInterpol::calcGcdEuclid()
{
    unsigned int gcd = mMAX( nrrows_-1, nrcols_-1 );
    unsigned int v = mMIN( nrrows_-1, nrcols_-1 );

    while ( v>0 )
    {
	unsigned int r  = gcd%v;
	gcd = v;
	v = r;
    }

    return  gcd;
}


int ContinuousCurvatureArray2DInterpol::calcPrimeFactors( int grid )
{
    nfact_ = 0;
    if ( grid<2 ) return grid;

    unsigned int curfactor = 0;
    const int basefactor[3] = { 2, 3, 5 };

    for ( int idx=0; idx<3; idx++ )
    {
	curfactor = basefactor[idx];
	while ( !(grid%curfactor) )
	{
	    grid /= curfactor;
	    factors_[nfact_++] = curfactor;
	}
	if ( grid==1 )
	    return grid;
    }

    bool twofourtoggle = false;
    bool tentwentytoggle = false;

    const unsigned int maxfactor =
	(unsigned int)mNINT32( Math::Floor(Math::Sqrt((double)grid)) );

    unsigned int skipfive = 25;
    while ( grid>1 && curfactor<=maxfactor )
    {
	if ( twofourtoggle )
	{
	    curfactor += 4;
	    twofourtoggle = false;
	}
	else
	{
	    curfactor += 2;
	    twofourtoggle = true;
	}

	if ( curfactor == skipfive )
	{
	    if ( tentwentytoggle )
	    {
		skipfive += 20;
		tentwentytoggle = false;
	    }
	    else
	    {
		skipfive += 10;
		tentwentytoggle = true;
	    }
	    continue;
	}

	while ( !(grid%curfactor) )
	{
	    grid /= curfactor;
	    factors_[nfact_++] = curfactor;
	}
    }

    if ( grid>1 )
	factors_[nfact_++] = grid;

    return grid;
}



void ContinuousCurvatureArray2DInterpol::updateEdgeConditions( int gridsize )
{
    const int padnrcols = nrcols_ + 2*mPadSize;
    const int grideast = gridsize*padnrcols;

    const int addw[5] = { -padnrcols,-grideast,-grideast,-grideast,-grideast };
    const int addw2[5] = { -2*padnrcols,-padnrcols-grideast,-2*grideast,
			   -2*grideast, -2*grideast };
    const int adde[5] = { grideast, grideast, grideast, grideast, padnrcols };
    const int adde2[5] = { 2*grideast,2*grideast,2*grideast,
			   padnrcols + grideast,2*padnrcols };
    const int adds[5] = { -1, -gridsize, -gridsize, -gridsize, -gridsize };
    const int adds2[5] = {-2,-gridsize-1,-2*gridsize,-2*gridsize,-2*gridsize};
    const int addn[5] = { gridsize, gridsize, gridsize, gridsize, 1 };
    const int addn2[5] = { 2*gridsize, 2*gridsize, 2*gridsize, gridsize+1, 2 };

    for ( int idx=0, globalidx=0; idx<5; idx++ )
    {
	for ( int idy=0; idy<5; idy++, globalidx++ )
	{
	    offset_[globalidx][0] = addn2[idy];
	    offset_[globalidx][1] = addn[idy] + addw[idx];
	    offset_[globalidx][2] = addn[idy];
	    offset_[globalidx][3] = addn[idy] + adde[idx];
	    offset_[globalidx][4] = addw2[idx];
	    offset_[globalidx][5] = addw[idx];
	    offset_[globalidx][6] = adde[idx];
	    offset_[globalidx][7] = adde2[idx];
	    offset_[globalidx][8] = adds[idy] + addw[idx];
	    offset_[globalidx][9] = adds[idy];
	    offset_[globalidx][10] = adds[idy] + adde[idx];
	    offset_[globalidx][11] = adds2[idy];
	}
    }
}


bool ContinuousCurvatureArray2DInterpol::setCoefficients()
{
    double loose = 1.0 - tension_;
    if ( loose==0 ) return false;

    const double e4 = cE2*cE2;
    const double a0 = 1.0/( (6*e4*loose + 10*cE2*loose + 8*loose -
	2*cOneplusE2) + 4*tension_*cOneplusE2 );

    coeff_[1][4] = coeff_[1][7] = -loose;
    coeff_[1][0] = coeff_[1][11] = -loose*e4;
    coeff_[0][4] = coeff_[0][7] = -loose*a0;
    coeff_[0][0] = coeff_[0][11] = -loose*e4*a0;
    coeff_[1][5] = coeff_[1][6] = 2*loose*cOneplusE2;
    coeff_[0][5] = coeff_[0][6] = (2*coeff_[1][5]+tension_)*a0;
    coeff_[1][2] = coeff_[1][9] = coeff_[1][5]*cE2;
    coeff_[0][2] = coeff_[0][9] = coeff_[0][5]*cE2;
    coeff_[1][1] = coeff_[1][3] = coeff_[1][8] = coeff_[1][10] = -2*loose*cE2;
    coeff_[0][1] = coeff_[0][3] = coeff_[0][8] = coeff_[0][10]=coeff_[1][1]*a0;

    return true;
}


void ContinuousCurvatureArray2DInterpol::findNearestPoint( int gridsize )
{
    const int padnrcols = nrcols_ + 2*mPadSize;
    const int blockny = ( nrcols_-1 )/gridsize + 1;
    const int rcswcorner = 2*padnrcols + 2;\

    for ( int idx=0; idx<nrrows_; idx+=gridsize )	/* Reset grid info */
	for ( int idy=0; idy<nrcols_; idy+= gridsize )
	    gridstatus_[rcswcorner+idx*padnrcols+idy] = 0;

    int briggsindex = 0;
    const double threshold = 0.05*gridsize;
    for ( int np=0, lastindex = UINT_MAX; np<nrdata_; np++ )
    {
	if ( hordata_[np].index_ != lastindex )
	{
	    const int blockr = (int)hordata_[np].index_/blockny;
	    const int blockc = (int)hordata_[np].index_%blockny;
	    lastindex = (int)hordata_[np].index_;
	    int iuindex=rcswcorner+(blockr*padnrcols+blockc)*gridsize;
	    double x0 = blockr*gridsize;
	    double y0 = blockc*gridsize;
	    double dx = (hordata_[np].x_-x0)/gridsize;
	    double dy = (hordata_[np].y_-y0)/gridsize;
	    if ( fabs(dx)<threshold && fabs(dy)<threshold )
	    {
		gridstatus_[iuindex] = 5;
		float zatnode = hordata_[np].z_ +
			(float)(gridsize*(planec1_*dx+planec2_*dy)/zscale_);
		    griddata_[iuindex] = zatnode;
	    }
	    else
	    {
		if ( dx>=0.0 )
		{
		    if ( dy>= 0.0 )
			gridstatus_[iuindex] = 1;
		    else
			gridstatus_[iuindex] = 4;
		}
		else
		{
		    if ( dy>= 0.0)
			gridstatus_[iuindex] = 2;
		    else
			gridstatus_[iuindex] = 3;
		}
		dx = fabs(dx);
		dy = fabs(dy);

		const BriggsData bd( dx, dy, hordata_[np].z_ );

		briggs_[1].b0_ = 0.1;
		briggs_[briggsindex] = bd;
		briggsindex++;
	    }
	 }
    }

}


ContinuousCurvatureArray2DInterpol::BriggsData::BriggsData(
    double dx, double dy, double z )
{
    // pre-const parameters calculation
    const double btemp = 2 * cOneplusE2 /( ( dx+dy )*( 1.0+dx+dy ) );
    const double xys = 1.0 + dx + dy;
    const double xy1 = 1.0 / xys;
    const double e2 = cE2*2;

    b0_ = 1.0 - 0.5*( dx+( dx*dx ) )*btemp;
    b3_ = 0.5 * ( e2-( dy+( dy*dy ) ) * btemp );

    b1_ = ( e2*xys-4*dy ) * xy1;
    b2_ = 2 * ( dy-dx+1.0 ) * xy1;
    b4_ = b0_ + b1_ + b2_ + b3_ + btemp;
    b5_ = btemp * z;
}


ContinuousCurvatureArray2DInterpol::BriggsData&
ContinuousCurvatureArray2DInterpol::BriggsData::operator =
( const ContinuousCurvatureArray2DInterpol::BriggsData& brgdata )
{
    if ( this != & brgdata )
    {
	b0_ = brgdata.b0_;
	b1_ = brgdata.b1_;
	b2_ = brgdata.b2_;
	b3_ = brgdata.b3_;
	b4_ = brgdata.b4_;
	b5_ = brgdata.b5_;
    }
    return *this;
}


bool ContinuousCurvatureArray2DInterpol::BriggsData::operator ==
( const ContinuousCurvatureArray2DInterpol::BriggsData& brgdata ) const
{
    return b0_== brgdata.b0_ && b1_ == brgdata.b1_ &&
	   b2_== brgdata.b2_ && b3_ == brgdata.b3_ &&
	   b4_== brgdata.b4_ && b5_ == brgdata.b5_;
}


ContinuousCurvatureArray2DInterpol::HorizonData&
ContinuousCurvatureArray2DInterpol::HorizonData::operator =
( const ContinuousCurvatureArray2DInterpol::HorizonData& hrdata )
{
    if ( this != & hrdata )
    {
	x_ = hrdata.x_;
	y_ = hrdata.y_;
	z_ = hrdata.z_;
	index_ = hrdata.index_;
    }
    return *this;
}


bool ContinuousCurvatureArray2DInterpol::HorizonData::operator ==
( const ContinuousCurvatureArray2DInterpol::HorizonData& hrdata ) const
{
    return x_ == hrdata.x_ &&
	   y_ == hrdata.y_ &&
	   z_ == hrdata.z_ &&
	   index_ == hrdata.index_;
}


void ContinuousCurvatureArray2DInterpol::updateGridIndex( int gridsize )
{
    const int blockny = (nrcols_-1)/gridsize + 1;
    for ( int idx=0; idx<nrdata_; idx++ )
    {
	const int i = mNINT32( Math::Floor((hordata_[idx].x_/gridsize) + 0.5) );
	const int j = mNINT32( Math::Floor((hordata_[idx].y_/gridsize) + 0.5) );
	hordata_[idx].index_ = i*blockny+j;
    }

    HorizonData* ptr = hordata_.ptr();
    std::sort( ptr, ptr+nrdata_, HorizonDataComparer(this,gridsize) );

}


int ContinuousCurvatureArray2DInterpol::doFiniteDifference( int gridsize )
{
    if ( tension_== 1 ) return -1;

    //Pre-calculate frequently used constants.
    const double a0const1 = 2.0 * (1.0-tension_) * (1.0+cE2*cE2);
    const double a0const2 = 2.0 - tension_ + 2*(1.0-tension_)*cE2;

    const double x0 = 4.0*(1.0-tension_ )/(2.0-tension_);
    const double x1 = (3*tension_-2.0)/(2.0-tension_);
    const double ydenom = 2*cLepsilon*(1.0-tension_) + tension_;
    const double y0 = 4*cLepsilon*(1.0-tension_)/ydenom;
    const double y1=(tension_-2*cLepsilon*(1.0-tension_))/ydenom;
    const double curlimit = convergelimit_/gridsize;
    const double relaxparam = 1.4;

    int iterationcount = 0;
    double maxchange = 0;
    const int padnrcols = nrcols_ + 2*mPadSize;
    mConstBlockSize();
    mConstCorners();

    do
    {
	int briggsidx = 0;
	maxchange = -1.0;

	for ( int r=0; r<nrrows_; r+=gridsize )
	{
	    int rc = rcswcorner+ r*padnrcols;
	    griddata_[rc-1] =
		(float)( y0*griddata_[rc]+y1*griddata_[rc+(int)gridsize] );

	    rc = rcnwcorner + r*padnrcols;
	    griddata_[rc+1] =
		(float)( y0*griddata_[rc]+y1*griddata_[rc-(int)gridsize] );
	}

        for ( int col=0; col<nrcols_; col+=(int)gridsize )
	{
	    int rc = rcswcorner+ col;
	    griddata_[rc-padnrcols] =
		(float)( x1*griddata_[rc+grideast] + x0*griddata_[rc] );
	    rc = rcsecorner + col;

	    griddata_[rc+padnrcols] =
		(float)( x1*griddata_[rc-grideast] + x0*griddata_[rc] );
	}

	int ij = rcswcorner;
	griddata_[ij-padnrcols-1] = griddata_[ij+grideast-1] +
	   griddata_[ij-padnrcols+gridsize] - griddata_[ij+grideast+gridsize];

	ij = rcnwcorner;
	griddata_[ij-padnrcols+1] = griddata_[ij+grideast+1] +
	   griddata_[ij-padnrcols-gridsize] - griddata_[ij+grideast-gridsize];

	ij = rcsecorner;
	griddata_[ij+padnrcols-1] = griddata_[ij-grideast-1] +
	   griddata_[ij+padnrcols+gridsize] - griddata_[ij-grideast+gridsize];

	ij = rcnecorner;
	griddata_[ij+padnrcols+1] = griddata_[ij-grideast+1] +
	   griddata_[ij+padnrcols-gridsize] - griddata_[ij-grideast-gridsize];


	for ( int row=0, xwcase=0, xecase=blocknx-1;
	      row<nrrows_; row+=gridsize, xwcase++, xecase-- )
	{
	    int xcase = 2;
	    if ( xwcase <2 )
		xcase = xwcase;
	    else if ( xecase<2 )
		xcase = 4 - xecase;

	    /* South side */
	    int kase = xcase*5;
	    int rc = rcswcorner + row*padnrcols;
	    float val0 = griddata_[rc+offset_[kase][0]];
	    float val1 = griddata_[rc+offset_[kase][1]] +
			 griddata_[rc+offset_[kase][3]] -
			 griddata_[rc+offset_[kase][8]] -
			 griddata_[rc+offset_[kase][10]];
	    float val2 = griddata_[rc+offset_[kase][9]] -
			 griddata_[rc+offset_[kase][2]];

	    griddata_[rc+offset_[kase][11]] =
		(float)( val0 + cEpsM2*val1 + cTwoplusM2*val2 );
	    /* North side */
	    kase = xcase * 5 + 4;
	    rc = rcnwcorner + row*padnrcols;
	    val0 = -griddata_[rc+offset_[kase][11]];
	    val1=griddata_[rc+offset_[kase][1]]+griddata_[rc+offset_[kase][3]]-
		 griddata_[rc+offset_[kase][8]]-griddata_[rc+offset_[kase][10]];
	    val2=griddata_[rc+offset_[kase][9]]-griddata_[rc+offset_[kase][2]];

	    griddata_[rc + offset_[kase][0]] =
		-(float)( val0 + cEpsM2*val1 + cTwoplusM2*val2 );
	}

	for ( int col=0, yscase = 0, yncase = blockny-1; col<nrcols_;
	    col+=gridsize, yscase++, yncase-- )
	{
	    int ycase = 2;
	    if ( yscase<2 )
		ycase = yscase;
	    else if ( yncase<2 )
		ycase = 4-yncase;

	    /* West side */
	    int kase = ycase;
	    int rc = rcswcorner + col;
	    float val0 = griddata_[rc+offset_[kase][7]] ;
	    float val1 = griddata_[rc+offset_[kase][3]] +
		         griddata_[rc+offset_[kase][10]]-
			 griddata_[rc+offset_[kase][1]] -
			 griddata_[rc+offset_[kase][8]];
	    float val2 = griddata_[rc+offset_[kase][5]] -
		         griddata_[rc+offset_[kase][6]];

	    griddata_[rc+offset_[kase][4]] =
		val0 + (float)cEpsP2*val1 + (float)cTwopluseP2*val2;

	    /* East side */
	    kase = 20 + ycase;
	    rc = rcsecorner + col;
	    val0 = -griddata_[rc+offset_[kase][4]] ;
	    val1=griddata_[rc+offset_[kase][3]]+griddata_[rc+offset_[kase][10]]-
		 griddata_[rc+offset_[kase][1]]-griddata_[rc+offset_[kase][8]];
	    val2=griddata_[rc+offset_[kase][5]]-griddata_[rc+offset_[kase][6]];

	    griddata_[rc+offset_[kase][7]] =
		-(float)( val0+cEpsP2*val1+cTwopluseP2*val2 );
	}

	for ( int row=0, xwcase=0, xecase=blocknx - 1;
	      row<nrrows_; row+=gridsize, xwcase++, xecase--)
	{
	    int xcase = 2;
	    if ( xwcase<2 )
		xcase = xwcase;
	    else if ( xecase<2 )
		xcase = 4-xecase;

	    for ( int col=0, yscase=0, yncase=blockny-1,
		rc=rcswcorner+row*padnrcols; col<nrcols_; col+=gridsize,
		rc+=gridsize, yscase++,
		yncase--)
	    {
		if ( gridstatus_[rc]==5 )
		    continue;

		int ycase = 2;
		if ( yscase<2 )
		    ycase = yscase;
		else if ( yncase<2 )
		    ycase = 4-yncase;

		const int kase = xcase*5 + ycase;
		double sumrc = 0.0;
		double busum = 0;
		if ( gridstatus_[rc]==0 )
		{
		    for ( int k=0; k<12; k++ )
			sumrc += (griddata_[rc+offset_[kase][k]]*coeff_[0][k]);
		}
		else
		{
		    BriggsData bd;
		    bd.b0_= briggs_[briggsidx].b0_;
		    bd.b1_ = briggs_[briggsidx].b1_;
		    bd.b2_ = briggs_[briggsidx].b2_;
		    bd.b3_ = briggs_[briggsidx].b3_;
		    bd.b4_ = briggs_[briggsidx].b4_;
		    bd.b5_ = briggs_[briggsidx].b5_;

		    briggsidx++;

		    if ( gridstatus_[rc]<3 )
		    {
			if ( gridstatus_[rc]==1 )
			{
			    /* Point is in quadrant 1  */
			    busum = bd.b0_*griddata_[rc+offset_[kase][10]] +
				    bd.b1_*griddata_[rc+offset_[kase][9]]  +
				    bd.b2_*griddata_[rc+offset_[kase][5]]  +
				    bd.b3_*griddata_[rc+offset_[kase][1]];
			}
			else
			{			/* Point is in quadrant 2  */
			    busum = bd.b0_*griddata_[rc+offset_[kase][8]] +
				    bd.b1_*griddata_[rc+offset_[kase][9]] +
			            bd.b2_*griddata_[rc+offset_[kase][6]] +
				    bd.b3_*griddata_[rc+offset_[kase][3]];
			}
		    }
		    else
		    {
			if ( gridstatus_[rc]==3 )
			{
			    /* Point is in quadrant 3  */
			    busum = bd.b0_*griddata_[rc+offset_[kase][1]] +
				    bd.b1_*griddata_[rc+offset_[kase][2]] +
				    bd.b2_*griddata_[rc+offset_[kase][6]] +
				    bd.b3_*griddata_[rc+offset_[kase][10]];
			}
			else
			{		/* Point is in quadrant 4  */
			    busum = bd.b0_*griddata_[rc+offset_[kase][3]] +
				    bd.b1_*griddata_[rc+offset_[kase][2]] +
				    bd.b2_*griddata_[rc+offset_[kase][5]] +
				    bd.b3_*griddata_[rc+offset_[kase][8]];
			}
		    }

		    for ( int k=0; k<12; k++ )
			sumrc += griddata_[rc + offset_[kase][k]]*coeff_[1][k];

		    sumrc = (sumrc+a0const2*(busum+bd.b5_)) /
			     (a0const1+a0const2*bd.b4_);
		}

		sumrc = griddata_[rc]*(1-relaxparam) + sumrc*relaxparam;
		const double change = fabs( sumrc-griddata_[rc] );
		griddata_[rc] = (float)sumrc;

		if ( change>maxchange )
		    maxchange = change;
	    }
	}

	iterationcount++;
	maxchange *= zscale_;

	addToNrDone( 1 );

    } while ( maxchange>curlimit && iterationcount<mMaxIterations );

    addToNrDone( mMaxIterations-iterationcount );

    return iterationcount;
}


void ContinuousCurvatureArray2DInterpol::fillInForecast(
    int oldgridsize, int curgridsize )
{
    const double oldsize = 1.0/(double)oldgridsize;
    const int padnrcols = nrcols_ + 2*mPadSize;
    mConstCorners();

    for ( int r=0; r<nrrows_-1; r+=oldgridsize )
    {
	for ( int c=0; c<nrcols_-1; c+=oldgridsize )
	{
	    const int idx0 = rcswcorner + r*padnrcols + c;
	    const int idx1 = idx0 + oldgridsize*padnrcols;
	    const int idx2 = idx1 + oldgridsize;
	    const int idx3 = idx0 + oldgridsize;

	    const double a0 = griddata_[idx0];
	    const double a1 = griddata_[idx1] - a0;
	    const double a2 = griddata_[idx3] - a0;
	    const double a3 = griddata_[idx2] - a0 - a1 - a2;

	    for ( int rr=r;  rr<r+oldgridsize; rr+=curgridsize )
	    {
		const double deltax = (rr-r)*oldsize;
		for ( int cc=c;  cc<c+oldgridsize; cc += curgridsize )
		{
		    const int idxnew = rcswcorner + rr*padnrcols + cc;

		    if ( idxnew==idx0 )
			continue;

		    const double deltay = (cc-c)*oldsize;
		    griddata_[idxnew] =
			(float)( a0+a1*deltax+deltay*(a2+a3*deltax) );
		    gridstatus_[idxnew] = 0;
		}
	    }
	    gridstatus_[idx0] = 5;
	}
    }

    for ( int c=0; c<nrcols_-1; c+=oldgridsize )
    {
	const int idx0 = rcsecorner + c;
	const int idx3 = idx0 + oldgridsize;
	for ( int cc=c;  cc<c+oldgridsize; cc += curgridsize )
	{
	    const int idxnew = rcsecorner + cc;
	    const double deltay = (cc-c)*oldsize;
	    griddata_[idxnew] = griddata_[idx0] + (float)(deltay *
		(griddata_[idx3]-griddata_[idx0]));
	    gridstatus_[idxnew] = 0;
	}
	gridstatus_[idx0] = 5;
    }

    for ( int r=0; r<nrrows_-1; r+=oldgridsize )
    {
	const int idx0 = rcnwcorner + r*padnrcols;
	const int idx1 = idx0 + oldgridsize*padnrcols;
	for ( int rr=r; rr<r+oldgridsize; rr += curgridsize )
	{
	    const int idxnew = rcnwcorner + rr*padnrcols;
	    const double deltax = (rr-r)*oldsize;
	    griddata_[idxnew] = griddata_[idx0] + (float)( deltax *
		(griddata_[idx1] - griddata_[idx0]) );
	    gridstatus_[idxnew] = 0;
	}
	gridstatus_[idx0] = 5;
    }

    gridstatus_[rcnecorner] = 5;
}


// below is temporal code for testing fault issue
void ContinuousCurvatureArray2DInterpol::InterpolatingFault(
    const TypeSet<HorizonData>& fdata, int gridsize )
{
    // this function is not complete ready
    TypeSet<HorizonData> bdata;
    int endidx = 0;
    for ( int idx=0, startidx=0; idx<fdata.size()-1; idx++ )
    {
	double dx = fdata[idx+1].x_ - fdata[idx].x_;
	double dy = fdata[idx+1].y_ - fdata[idx].y_;
	double dz = fdata[idx+1].z_ - fdata[idx].z_;
	const int deltasize = mNINT32( mMAX(fabs(dx), fabs(dy)) ) + 1;
	endidx += deltasize;
	dx /= ( Math::Floor((double)deltasize) - 1 );
	dy /= ( Math::Floor((double)deltasize) - 1 );
	dz /= ( Math::Floor((double)deltasize) - 1 );
	for ( od_int64 k = startidx, n = 0; k < endidx - 1; k++,n++)
	{
	    HorizonData hd;
	    hd.x_ = fdata[idx].x_ + n*(float)dx;
	    hd.y_ = fdata[idx].y_ + n*(float)dy;
	    hd.z_ = fdata[idx].z_ + n*(float)dz;
	    bdata.add( hd );
	}
	HorizonData hd;
	hd.x_ = fdata[idx+1].x_;
	hd.y_ = fdata[idx+1].y_;
	hd.z_ = fdata[idx+1].z_;
	bdata.add( hd );

	startidx += deltasize;
    }

    int curcount = nrdata_;
    zmean_ *= curcount;

    const int blocknx = ( nrrows_-1 )/gridsize + 1;
    const int blockny = ( nrcols_-1 )/gridsize + 1;

    for ( int idx=0; idx<endidx; idx++)
    {
	const int scol = mNINT32( Math::Floor(bdata[idx].x_+0.5) );
	if ( scol<0 || scol >= blocknx )
	    continue;
	const int srow = mNINT32( Math::Floor(bdata[idx].y_+0.5) );
	if ( srow<0 || srow >=blockny )
	    continue;

	HorizonData hd;
	hd.x_ = bdata[idx].x_;
	hd.y_ = bdata[idx].y_;
	hd.z_ = bdata[idx].z_;
	hd.index_ = scol*blockny+srow;
	//hordata_.add( hd );
	curcount++;
	zmean_ += bdata[idx].z_;
    }

    zmean_ /= curcount;

}
