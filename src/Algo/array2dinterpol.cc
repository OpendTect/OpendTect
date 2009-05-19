/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2009
-*/

static const char* rcsID = "$Id: array2dinterpol.cc,v 1.12 2009-05-19 21:57:57 cvskris Exp $";

#include "array2dinterpolimpl.h"

#include "arrayndimpl.h"
#include "delaunay.h"
#include "polygon.h"
#include "limits.h"
#include "rowcol.h"
#include "sorting.h"
#include "trigonometry.h"


#define mPolygonType int

DefineEnumNames( Array2DInterpol, FillType, 1, "Filltypes" )
{ "Only Holes", "Convex Hull", "Full", 0 };

mImplFactory( Array2DInterpol, Array2DInterpol::factory );

Array2DInterpol::Array2DInterpol()
    : arr_( 0 )
    , arrsetter_( 0 )
    , nrcells_( -1 )
    , nrrows_( -1 )
    , nrcols_( -1 )
    , filltype_( Full )
    , maxholesize_( mUdf(float) )
    , rowstep_( 1 )
    , colstep_( 1 )
    , mask_( 0 )
    , maskismine_( false )
{}


Array2DInterpol::~Array2DInterpol()
{
    if ( maskismine_ ) delete mask_;
}


void Array2DInterpol::setFillType( FillType ft )
{ filltype_ = ft; arr_ = 0; arrsetter_=0; }


void Array2DInterpol::setRowStep( float rs )
{ rowstep_ = rs; arr_ = 0; arrsetter_ = 0; }


void Array2DInterpol::setColStep( float cs )
{ colstep_ = cs; arr_ = 0; arrsetter_ = 0; }


Array2DInterpol::FillType Array2DInterpol::getFillType() const
{ return filltype_; }


void Array2DInterpol::setMaxHoleSize( float maxholesize )
{ maxholesize_ = maxholesize; arr_ = 0; arrsetter_=0; }


float Array2DInterpol::getMaxHoleSize() const
{ return maxholesize_; }


void Array2DInterpol::setMask( const Array2D<bool>* mask, OD::PtrPolicy policy )
{
    if ( maskismine_ )
	delete mask_;

    arr_ = 0; arrsetter_=0; mask_ = 0;

    if ( policy==OD::CopyPtr )
    {
	if ( mask ) mask_ = new Array2DImpl<bool>( *mask );
	maskismine_ = true;
    }
    else 
    {
	mask_ = mask;
	maskismine_ = policy==OD::TakeOverPtr;
    }
}


bool Array2DInterpol::setArray( Array2D<float>& arr, TaskRunner* )
{
    arr_ = &arr;
    arrsetter_ = 0;
    nrrows_ = arr.info().getSize(0);
    nrcols_ = arr.info().getSize(1);
    nrcells_ = nrrows_*nrcols_;

    return true;
}


bool Array2DInterpol::setArray( ArrayAccess& arr, TaskRunner* )
{
    if ( !canUseArrayAccess() )
	return false;

    arr_ = 0;
    arrsetter_ = &arr;
    nrrows_ = arr.getSize(0);
    nrcols_ = arr.getSize(1);
    nrcells_ = nrrows_*nrcols_;

    return true;
}


#define mUpdateRange( r, c ) \
if ( !rgset ) \
{ \
    rgset = true; \
    rowrg.start = rowrg.stop = r; \
    colrg.start = colrg.stop = c; \
} \
else \
{ \
    rowrg.include( r ); \
    colrg.include( c ); \
}


void Array2DInterpol::getNodesToFill( const bool* def,
				      bool* shouldinterpol,
				      TaskRunner* tr ) const
{
    ArrPtrMan<bool> owndef;
    if ( !def )
    {
	mTryAlloc( owndef, bool[nrcells_] );
	if ( !owndef )
	    return;

	bool* ptr = owndef.ptr();
	const bool* stopptr = ptr+nrcells_;
	int idx = 0;
	while ( ptr!=stopptr )
	{
	    *ptr = isDefined( idx++ );
	    ptr++;
	}

	def = owndef.ptr();
    }

    bool initialstate = true;

    MemSetter<bool> setter( shouldinterpol, filltype_!=ConvexHull,
	    		    nrcells_ );
    setter.execute();

    if ( filltype_==ConvexHull )
    {
	ODPolygon<mPolygonType> poly;
	Interval<int> rowrg, colrg;
	bool rgset = false;

	for ( int icol=0; icol<nrcols_; icol++ )
	{
	    int idx = icol;
	    for ( int irow=0; irow<nrrows_; irow++, idx+=nrcols_ )
	    {
		if ( def[idx] ) 
		{
		    poly.add( Geom::Point2D<mPolygonType>(irow,icol) );
		    mUpdateRange( irow, icol );

		    idx = (nrrows_-1)*nrcols_+icol;
		    for ( int jrow=nrrows_-1; jrow>irow; jrow--, idx-= nrcols_ )
		    {
			if ( def[idx] )
			{
			    poly.add( Geom::Point2D<mPolygonType>(jrow,icol) );
			    mUpdateRange( jrow, icol );
			    break;
			}
		    }
		    break;
		}
	    }
	}

	if ( !rgset )
	    return;

	poly.convexHull();

	//For each col, figure out start/stop row for convex hull
	Geom::Point2D<mPolygonType> pt;
	int prevstart=rowrg.start, prevstop=rowrg.stop;
	for ( int icol=colrg.start; icol<=colrg.stop; icol++ )
	{
	    pt.y = icol;
	    int irow = prevstart;
	    pt.x = irow;
	    bool hadaninside = false;
	    bool isinside;
	    while ( true )
	    {
		isinside = poly.isInside( pt, true, 0 );
		if ( !isinside )
		    break;

		hadaninside = true;

		if ( !rowrg.includes(irow) ) break;

		irow--;
		pt.x = irow;
	    }

	    if ( !hadaninside || !isinside )
		irow++;

	    if ( !hadaninside )
	    {
		pt.x = irow;
		while ( !poly.isInside( pt, true, 0 ) )
		{
		    irow++;
		    pt.x = irow;
		}
	    }

	    Interval<int> thisrowrg;
	    thisrowrg.start = prevstart = irow;
	    irow = prevstop;
	    pt.x = irow;
	    hadaninside = false;

	    while ( true )
	    {
		isinside = poly.isInside( pt, true, 0 );
		if ( !isinside )
		    break;

		hadaninside = true;

		if ( !rowrg.includes(irow) ) break;

		irow++;
		pt.x = irow;
	    }

	    if ( !hadaninside || !isinside )
		irow--;

	    if ( !hadaninside )
	    {
		pt.x = irow;
		while ( !poly.isInside( pt, true, 0 ) )
		{
		    irow--;
		    pt.x = irow;
		}
	    }

	    thisrowrg.stop = prevstop = irow;

	    int idx = icol+thisrowrg.start*nrcols_;
	    for ( irow=thisrowrg.start; irow<=thisrowrg.stop;
		    irow++, idx+= nrcols_ )
	    {
		if ( def[idx] ) 
		    continue;

		shouldinterpol[idx] = true;
	    }
	}
    }
    else if ( filltype_==HolesOnly )
    {
	for ( int icol=0; icol<nrcols_; icol++ )
	{
	    int idx = icol;
	    if ( !def[idx] )
		floodFillArrFrom( idx, def, shouldinterpol );

	    idx = (nrrows_-1)*nrcols_+icol;
	    if ( !def[idx] )
		floodFillArrFrom( idx, def, shouldinterpol );
	}

	for ( int irow=1; irow<nrrows_-1; irow++ )
	{
	    int idx = irow*nrcols_;
	    if ( !def[idx] ) 
		floodFillArrFrom( idx, def, shouldinterpol );

	    idx = (irow+1)*nrcols_-1;
	    if ( !def[idx] ) 
		floodFillArrFrom( idx, def, shouldinterpol );
	}
    }

    excludeBigHoles( def, shouldinterpol );

    if ( !mask_ )
	return;

    if ( mask_->info().getSize(0)==nrrows_ &&
	 mask_->info().getSize(1)==nrcols_ &&
	 (mask_->getData() || mask_->getStorage() ) )
    {
	const bool* maskptr = mask_->getData();
	if ( maskptr )
	{
	    for ( int idx=0; idx<nrcells_; idx++ )
	    {
		if ( !shouldinterpol[idx] ) continue;
		shouldinterpol[idx] = maskptr[idx];
	    }
	}
	else
	{
	    const ValueSeries<bool>* stor = mask_->getStorage();
	    for ( int idx=0; idx<nrcells_; idx++ )
	    {
		if ( !shouldinterpol[idx] ) continue;
		shouldinterpol[idx] = stor->value(idx);
	    }
	}
    }
    else
    {
	const int masknrrows = mMIN(nrrows_,mask_->info().getSize(0) );
	const int masknrcols = mMIN(nrcols_,mask_->info().getSize(1) );
	for ( int irow=0; irow<masknrrows; irow++ )
	{
	    int offset = irow * nrcols_;
	    for ( int icol=0; icol<masknrcols; icol++, offset++ )
	    {
		if ( !shouldinterpol[offset] ) continue;
		shouldinterpol[offset] = mask_->get( irow, icol );
	    }
	}
    }
}


#define mGoToNeighbor( nextvar ) \
    next = nextvar; \
    if ( next>=0 && next<nrcells_ ) \
    { \
	if ( shouldinterpol[next] && !def[next] ) \
	{ \
	    shouldinterpol[next] = false; \
	    seeds += next; \
	} \
    }


bool Array2DInterpol::isDefined( int idx ) const
{
    if ( arrsetter_ )
	return arrsetter_->isDefined(idx);

    float val;

    const float* ptr = arr_->getData();
    if ( ptr ) val = ptr[idx];
    else
    {
	ValueSeries<float>* storage = arr_->getStorage();
	if ( storage )
	    val = storage->value( idx );
	else
	{
	    const int row = idx/nrcols_;
	    const int col = idx%nrcols_;
	    val = arr_->get( row, col );
	}
    }

    return !mIsUdf( val );
}

#define mDoLoop( fetchval ) \
for ( int idx=0; idx<nrsrc; idx++ ) \
{ \
    fetchval; \
    const float weight = weights[idx]; \
    sum += val*weight; \
    wsum += weight; \
}


void Array2DInterpol::setFrom( int target, const int* sources,
			       const float* weights, int nrsrc)
{
    if ( !nrsrc )
	return;

    if ( arrsetter_ )
    {
	arrsetter_->set( target, sources, weights, nrsrc );
	return;
    }

    double sum = 0;
    double wsum = 0;

    float* ptr = arr_->getData();
    if ( ptr )
    {
	mDoLoop( const float val = ptr[sources[idx]] );
	ptr[target] = sum/wsum;
    }
    else
    {
	ValueSeries<float>* storage = arr_->getStorage();
	if ( storage )
	{
	    mDoLoop( const float val = storage->value(sources[idx]) );
	    storage->setValue(target, sum/wsum );
	}
	else
	{
	    mDoLoop( const int src = sources[idx];
		     const float val = arr_->get(src/nrcols_,src%nrcols_ ) );
	    arr_->set( target/nrcols_, target%nrcols_, sum/wsum );
	}
    }
}


void Array2DInterpol::floodFillArrFrom( int seed, const bool* def,
				 	bool* shouldinterpol ) const
{
    shouldinterpol[seed] = false;

    int next;

    TypeSet<int> seeds( 1, seed );
    while ( seeds.size() )
    {
	const int curseedidx = seeds.size()-1;
	const int curseed = seeds[curseedidx];
	seeds.remove( curseedidx, false );

	mGoToNeighbor( curseed-nrcols_ ); //Prev row
	mGoToNeighbor( curseed+nrcols_ ); //Next row

	const int col = curseed%nrcols_;

	if ( col )
	    mGoToNeighbor( curseed-1 ); //Prev col
	if ( col!=nrcols_-1 )
	    mGoToNeighbor( curseed+1 ); //Next col

    }
}


void Array2DInterpol::excludeBigHoles( const bool* def,
				       bool* shouldinterpol ) const
{
    if ( mIsUdf(maxholesize_) )
	return;

    const int maxrowsize = (int) ceil( maxholesize_/rowstep_ );
    const int maxcolsize = (int) ceil( maxholesize_/colstep_ );

    for ( int irow=0; irow<nrrows_; irow++ )
    {
	int nrtobeinterp = 0;
	for ( int icol=0; icol<nrcols_; icol++ )
	{
	    const int idx = irow*nrcols_ + icol;
	    if ( !def[idx] && shouldinterpol[idx] )
		nrtobeinterp++;

	    if ( nrtobeinterp>maxcolsize )
	    {
		floodFillArrFrom( idx, def, shouldinterpol );
		nrtobeinterp = 0;
	    }
	}
    }
    for ( int icol=0; icol<nrcols_; icol++ )
    {
	int nrtobeinterp = 0;
	for ( int irow=0; irow<nrrows_; irow++ )
	{
	    const int idx = irow*nrcols_ + icol;
	    if ( !def[idx] && shouldinterpol[idx] )
		nrtobeinterp++;

	    if ( nrtobeinterp>maxrowsize )
	    {
		floodFillArrFrom( idx, def, shouldinterpol );
		nrtobeinterp = 0;
	    }
	}
    }
}



//InverseDistance
void InverseDistanceArray2DInterpol::initClass()
{ Array2DInterpol::factory().addCreator( create, sType() ); }


Array2DInterpol* InverseDistanceArray2DInterpol::create()
{
    return new InverseDistanceArray2DInterpol;
}


InverseDistanceArray2DInterpol::InverseDistanceArray2DInterpol()
    : searchradius_( 10 )
    , stepidx_( -1 )
    , nrsteps_( mUdf(int) )
    , shouldend_( false )
    , stepsize_( 1 )
    , cornersfirst_( false )
    , nrinitialdefined_( -1 )
    , nodestofill_( 0 )
    , nrthreadswaiting_( 0 )
    , waitforall_( false )
    , curdefined_( 0 )
    , totalnr_( -1 )
    , nrthreads_( mUdf(int) )
    , nraddedthisstep_( mUdf(int) )
    , prevsupportsize_( mUdf(int) )
{}


InverseDistanceArray2DInterpol::~InverseDistanceArray2DInterpol()
{
    delete [] nodestofill_;
    delete [] curdefined_;
}


bool InverseDistanceArray2DInterpol::setArray( Array2D<float>& arr,
					       TaskRunner* tr )
{
    if ( !Array2DInterpol::setArray(arr, tr ) )
	return false;

    return initFromArray( tr );
}


bool InverseDistanceArray2DInterpol::setArray( ArrayAccess& arr,
					       TaskRunner* tr )
{
    if ( !Array2DInterpol::setArray(arr, tr ) )
	return false;

    return initFromArray( tr );
}


bool InverseDistanceArray2DInterpol::initFromArray( TaskRunner* tr )
{
    if ( !arr_ && !arrsetter_ )
	return false;

    nrinitialdefined_ = 0;

    delete [] curdefined_;
    mTryAlloc( curdefined_, bool[nrcells_] );
    if ( !curdefined_ )
	return false;

    bool* ptr = curdefined_;
    const bool* stopptr = ptr+nrcells_;
    int idx = 0;
    while ( ptr!=stopptr )
    {
	const bool isdef = isDefined( idx++ );
	*ptr = isdef;
	if ( isdef ) nrinitialdefined_++;
	ptr++;
    }

    if ( !nrinitialdefined_ )
	return false;

    delete [] nodestofill_;
    mTryAlloc( nodestofill_, bool[nrcells_] );
    if ( !nodestofill_ )
	return false;

    getNodesToFill( curdefined_, nodestofill_, tr );

    totalnr_ = 0;
    ptr = curdefined_;
    const bool* nodestofillptr = nodestofill_;
    while ( ptr!=stopptr )
    {
	if ( *nodestofillptr && !*ptr )
	    totalnr_++;

	ptr++;
	nodestofillptr++;
    }

    return true;
}


bool InverseDistanceArray2DInterpol::doPrepare( int nrthreads )
{
    nrthreads_ = nrthreads;
    if ( !nrinitialdefined_ )
	return false; //Nothing defined;

    if ( mIsUdf(searchradius_) )
    {
	const bool* ptr = curdefined_;
	for ( int idx=0; idx<nrcells_; idx++, ptr++ )
	{
	    if ( *ptr ) definedidxs_ += idx;
	}
    }
    else
    {
	const int rowradius = (int) ceil( searchradius_/rowstep_ );
	const int colradius = (int) ceil( searchradius_/colstep_ );

	float radius2 = searchradius_*searchradius_;
	neighbors_.erase();
	neighborweights_.erase();

	for ( int relrow=0; relrow<=rowradius; relrow++ )
	{
	    const float frelrow = relrow*rowstep_;
	    const float rowdist2 = frelrow*frelrow;

	    for ( int relcol=0; relcol<=colradius; relcol++ )
	    {
		if ( !relrow && !relcol)
		    continue;

		const float frelcol = relcol*colstep_;
		const int coldist2 = mNINT(frelcol*frelcol);
		const float dist2 = coldist2+rowdist2;
		if ( dist2>radius2 )
		    continue;

		const float weight = 1.0/Math::Sqrt( dist2 );

		neighbors_ += RowCol(relrow,relcol);
		neighborweights_ += weight;
		neighbors_ += RowCol(-relrow,-relcol);
		neighborweights_ += weight;
		if ( relrow && relcol )
		{
		    neighbors_ += RowCol(-relrow,relcol);
		    neighborweights_ += weight;
		    neighbors_ += RowCol(relrow,-relcol);
		    neighborweights_ += weight;
		}
	    }
	}
    }

    stepidx_ = -1;
    shouldend_ = false;
    todothisstep_.erase();
    nrsources_.erase();
    nraddedthisstep_ = 0;

    return true;
}


#define mGetOffset( r, c ) ((r)*nrcols_+(c))

bool InverseDistanceArray2DInterpol::doWork( od_int64, od_int64, int)
{
    ArrPtrMan<float> weights = 0;
    ArrPtrMan<int> sources = 0;

    int rowradius,colradius;

    if ( definedidxs_.size() ) //No search radius, do all pts
    {
	mTryAlloc( weights, float[definedidxs_.size()] );
	if ( !weights )
	    return false;
    }
    else
    {
	rowradius = (int) ceil( searchradius_/rowstep_ );
	colradius = (int) ceil( searchradius_/colstep_ );

	const int maxnr = (rowradius*2+1)*(colradius*2+1)-1;

	mTryAllocPtrMan( weights, float[maxnr] );
	mTryAllocPtrMan( sources, int[maxnr] );
	if ( !weights )
	    return false;
    }

    while ( true )
    {
	const od_int64 idx = getNextIdx();
	if ( idx<0 )
	    break;

	int targetrow = idx/nrcols_, targetcol = idx%nrcols_;

	if ( definedidxs_.size() ) //No search radius, do all pts
	{
	    for ( int idy=definedidxs_.size()-1; idy>=0; idy-- )
	    {
		const int source = definedidxs_[idy];
		const float sourcerow = source/nrcols_;
		const float sourcecol = source%nrcols_;

		const float rowdist = (targetrow-sourcerow)*rowstep_;
		const float rowdist2 = rowdist*rowdist;
		const float coldist = (targetcol-sourcecol)*colstep_;
		const int coldist2 = mNINT(coldist*coldist);
		const float weight = 1/Math::Sqrt( coldist2+rowdist2 );

		weights[idy] = weight;
	    }

	    setFrom( idx, definedidxs_.arr(), weights, definedidxs_.size() );
	}
	else
	{
	    int nrsources = 0;
	    for ( int neighbor=neighbors_.size()-1; neighbor>=0; neighbor-- )
	    {
		const RowCol& rc = neighbors_[neighbor];
		const int sourcerow = targetrow+rc.row;
		if ( sourcerow<0 || sourcerow>=nrrows_ )
		    continue;

		const int sourcecol = targetcol+rc.col;
		if ( sourcecol<0 || sourcecol>=nrcols_ )
		    continue;

		const int sourceidx = mGetOffset(sourcerow,sourcecol);
		if ( curdefined_[sourceidx] )
		{
		    sources[nrsources] = sourceidx;
		    weights[nrsources] = neighborweights_[neighbor];
		    nrsources++;
		}
	    }

	    if ( !nrsources )
		continue;

	    setFrom( idx, sources, weights, nrsources );
	}

	reportDone( idx );

	if ( !shouldContinue() )
	    return false;
    }

    return true;
}


class InvDistArr2DGridFindSources : public ParallelTask
{
public:
    InvDistArr2DGridFindSources( const bool* def, const bool* nodestofill,
	    			 int nrrows, int nrcols,
				 TypeSet<int>& res, TypeSet<int>& nrs,
				 int stepsize, bool cornersfirst )
	    : nrcells_( nrrows * nrcols )
	    , nrrows_( nrrows )
	    , nrcols_( nrcols )
	    , curdefined_( def )
	    , res_( res )
	    , nrsourceslist_( nrs )
	    , stepsize_( stepsize )
	    , maxnrsources_( 0 )
	    , cornersfirst_( cornersfirst )
	    , nodestofill_( nodestofill )
    {
	for ( int idx=-stepsize_; idx<=stepsize_; idx++ )
	{
	    const Interval<int> rg(idx*nrcols_-stepsize_,idx*nrcols_+stepsize_);
	    idxs_ += rg;
	}
    }

    od_int64 nrIterations() const { return nrcells_; }

    bool doWork( od_int64 start, od_int64 stop, int )
    {
	int maxnrsources = 0;
	TypeSet<int> localres;
	TypeSet<int> nrsourceslist;

	for ( int idx=start; idx<=stop; idx++ )
	{
	    if ( curdefined_[idx] || !nodestofill_[idx] )
		continue;

	    const int col = idx%nrcols_;

	    int nrsources = 0;

	    for ( int idy=-stepsize_; idy<=stepsize_; idy++ )
	    {
		const int targetcol = col+idy;
		if ( targetcol<0 )
		{
		    idy -= targetcol+1;
		    continue;
		}

		if ( targetcol>=nrcols_ )
		    break;

		for ( int idz=-stepsize_; idz<=stepsize_; idz++ )
		{
		    const int offset = idx+idy+idz*nrcols_;

		    if ( offset<0 )
		    {
			idz -= offset+1;
			continue;
		    }

		    if ( offset>=nrcells_ )
			break;

		    if ( curdefined_[offset] )
			nrsources++;
		}
	    }

	    if ( !nrsources )
		continue;

	    if ( cornersfirst_ )
	    {
		if ( nrsources<maxnrsources )
		    continue;

		if ( nrsources>maxnrsources )
		{
		    localres.erase();
		    nrsourceslist.erase();
		    maxnrsources = nrsources;
		}
	    }

	    nrsourceslist += nrsources;
	    localres += idx;
	}

	lock_.lock();
	if ( cornersfirst_ )
	{
	    if ( maxnrsources<maxnrsources_ )
	    {
		lock_.unLock();
		return true;
	    }

	    if ( maxnrsources>maxnrsources_ )
	    {
		res_.erase();
		nrsourceslist_.erase();
		maxnrsources_ = maxnrsources;
	    }
	}

	nrsourceslist_.append( nrsourceslist );
	res_.append( localres );

	lock_.unLock();
	return true;
    }

    bool doFinish( bool success )
    {
	if ( !success )
	    return false;

	if ( !cornersfirst_ )
	{
	    sort_coupled( nrsourceslist_.arr(), res_.arr(),
		    	  nrsourceslist_.size() );
	}

	return true;
    }

protected:
    TypeSet<Interval<int> >	idxs_;

    int				nrcells_;
    int				nrrows_;
    int				nrcols_;
    const bool*			curdefined_;
    const bool*			nodestofill_;
    TypeSet<int>&		res_;
    TypeSet<int>&		nrsourceslist_;
    int				stepsize_;
    int				maxnrsources_;
    bool			cornersfirst_;

    Threads::Mutex		lock_;
};

#define mRet( retval ) \
{ \
    if ( waitforall_ ) \
    { \
	waitforall_ = false; \
	condvar_.signal( true ); \
    } \
    return retval; \
}

od_int64 InverseDistanceArray2DInterpol::getNextIdx()
{
    Threads::MutexLocker lock( condvar_ );

    //Is someone else updating the step? If so, wait
    if ( waitforall_ )
    {
	nrthreadswaiting_++;
	condvar_.signal( true ); //Tell epoch-updating thread that I'm here.
	while ( waitforall_ )
	    condvar_.wait();
	nrthreadswaiting_--;
    }

    if ( shouldend_ ) //someone else has detected there's nothing more to do
    {
	mRet(-1);
    }

    int nrleft;
    while ( true )
    {
	nrleft =  todothisstep_.size();
	if ( nrleft )
	    break;

	//If all steps are used or there is no searchradius and one step is used
	if ( (!mIsUdf(nrsteps_) && stepidx_==nrsteps_-1 ) ||
	      ( definedidxs_.size() && !stepidx_ ) )
	{
	    shouldend_ = true;
	    mRet(-1);
	}

	//We need to wait here for every trace to finish old step
	waitforall_ = true;
	while ( nrthreadswaiting_<nrthreads_-1 )
	    condvar_.wait();

	//Check if anything changed last step, if no, quit.
	if ( stepidx_!=-1 && !nraddedthisstep_ )
	{
	    shouldend_ = true;
	    mRet( -1 );
	}

	//Update defined array
	for ( int idx=addedwithcursuport_.size()-1; idx>=0; idx-- )
	    curdefined_[addedwithcursuport_[idx]] = true;

	nraddedthisstep_ = 0;
	stepidx_++;

	//Create new step
	if ( definedidxs_.size() ) //without searchradius
	{
	    todothisstep_.erase();
	    for ( int idx=0; idx<nrcells_; idx++ )
	    {
		if ( !curdefined_[idx] || nodestofill_[idx] )
		    todothisstep_ += idx;
	    }

	    continue;
	}

	//find all nodes within stepsize_ from defined nodes, and order 
	//by nr of defined nrnodes inside -stepsize to stepsize
	addedwithcursuport_.erase();
	InvDistArr2DGridFindSources finder( curdefined_, nodestofill_,
				nrrows_, nrcols_,
				todothisstep_, nrsources_,
				stepsize_, cornersfirst_ );
	finder.execute();

	if ( !todothisstep_.size() )
	{
	    shouldend_ = true;
	    mRet( -1 );
	}

	prevsupportsize_ = mUdf(int);
    }

    //Take the last item from list
    const int res = todothisstep_[nrleft-1];
    todothisstep_.remove( nrleft-1 );
    if ( nrsources_.size() )
    {
	//If nr of node support changed since last pos, update list
	if ( !mIsUdf(prevsupportsize_) &&
		prevsupportsize_!=nrsources_[nrleft-1] )
	{
	    waitforall_ = true;
	    while ( nrthreadswaiting_<nrthreads_-1 )
		condvar_.wait();

	    for ( int idx=addedwithcursuport_.size()-1; idx>=0; idx-- )
		curdefined_[addedwithcursuport_[idx]] = true;

	    addedwithcursuport_.erase();
	}

	prevsupportsize_ = nrsources_[nrleft-1];
	nrsources_.remove( nrleft-1 );
    }

    mRet( res );
}

#undef mRet


void InverseDistanceArray2DInterpol::reportDone( od_int64 idx )
{
    addToNrDone( 1 );
    Threads::MutexLocker lock( condvar_ );
    addedwithcursuport_ += idx;
    nraddedthisstep_++;
}


// Triangulation
void TriangulationArray2DInterpol::initClass()
{ Array2DInterpol::factory().addCreator( create, sType() ); }


Array2DInterpol* TriangulationArray2DInterpol::create()
{
    return new TriangulationArray2DInterpol;
}


TriangulationArray2DInterpol::TriangulationArray2DInterpol()
    : triangulation_( 0 )
    , curdefined_( 0 )
    , nodestofill_( 0 )
    , totalnr_( -1 )
{}


TriangulationArray2DInterpol::~TriangulationArray2DInterpol()
{
    delete [] nodestofill_;
    delete [] curdefined_;
}


bool TriangulationArray2DInterpol::setArray( Array2D<float>& arr,
					       TaskRunner* tr )
{
    if ( !Array2DInterpol::setArray(arr, tr ) )
	return false;

    return initFromArray( tr );
}


bool TriangulationArray2DInterpol::setArray( ArrayAccess& arr,
					       TaskRunner* tr )
{
    if ( !Array2DInterpol::setArray(arr, tr ) )
	return false;

    return initFromArray( tr );
}


#define mSetRange \
if ( !rgset ) \
{ \
    rgset = true; \
    xrg.start = xrg.stop = row; \
    yrg.start = yrg.stop = col; \
} \
else \
{ \
    xrg.include( row ); \
    yrg.include( col ); \
}


bool TriangulationArray2DInterpol::initFromArray( TaskRunner* tr )
{
    if ( !arr_ && !arrsetter_ )
	return false;

    delete [] curdefined_;
    mTryAlloc( curdefined_, bool[nrcells_] );
    if ( !curdefined_ )
	return false;

    bool* ptr = curdefined_;
    const bool* stopptr = ptr+nrcells_;
    int idx = 0;
    while ( ptr!=stopptr )
    {
	const bool isdef = isDefined( idx++ );
	*ptr = isdef;
	ptr++;
    }

    delete [] nodestofill_;
    mTryAlloc( nodestofill_, bool[nrcells_] );
    if ( !nodestofill_ )
	return false;

    getNodesToFill( curdefined_, nodestofill_, tr );

    totalnr_ = 0;
    ptr = curdefined_;
    idx = 0;
    const bool* nodestofillptr = nodestofill_;
    Interval<int> xrg, yrg;
    bool rgset = false;
    while ( ptr!=stopptr )
    {
	if ( *nodestofillptr && !*ptr )
	{
	    const int row = idx/nrcols_;
	    const int col = idx%nrcols_;
	    totalnr_++;

	    mSetRange;
	}

	ptr++;
	nodestofillptr++;
	idx++;
    }

    //Get defined nodes to triangulate
    coordlist_.erase();
    coordlistindices_.erase();
    ptr = curdefined_;
    idx = 0;
    while ( ptr!=stopptr )
    {
	if ( *ptr )
	{
	    bool dotriangulate = false;
	    const int row = idx/nrcols_;
	    const int col = idx%nrcols_;
	    const bool isnotlastcol = col!=nrcols_-1;

	    if ( row )
	    {
		if ( (col && !curdefined_[idx-nrcols_-1]) ||
		     !curdefined_[idx-nrcols_] ||
		     (isnotlastcol && !curdefined_[idx-nrcols_+1]) )
		    dotriangulate = true;
	    }

	    if ( !dotriangulate && row!=nrrows_-1 )
	    {
		if ( (col && !curdefined_[idx+nrcols_-1]) ||
		     !curdefined_[idx+nrcols_] ||
		     (isnotlastcol && !curdefined_[idx+nrcols_+1]) )
		    dotriangulate = true;
	    }

	    if ( !dotriangulate )
	    {
		if ( (col && !curdefined_[idx-1]) ||
		     (isnotlastcol && !curdefined_[idx+1]) )
		    dotriangulate = true;
	    }

	    if ( dotriangulate )
	    {
		mSetRange;
		const Coord crd(rowstep_*row, colstep_*col);
		coordlist_ += crd;
		coordlistindices_ += idx;
	    }
	}

	idx++;
	ptr++;
    }

    if ( coordlist_.isEmpty() )
	return false;

    if ( triangulation_ )
	delete triangulation_;

    triangulation_ = new DAGTriangleTree;
    if ( !triangulation_ ||
	 !triangulation_->setCoordList( &coordlist_, OD::UsePtr ) )
	return false;

    if ( !triangulation_->setBBox(
		Interval<double>( xrg.start*rowstep_, xrg.stop*rowstep_ ),
		Interval<double>( yrg.start*colstep_, yrg.stop*colstep_ ) ) )
	return false;

    ParallelDTriangulator triangulator( *triangulation_ );
    triangulator.dataIsRandom( false );
    triangulator.setCalcScope( Interval<int>( 0, coordlist_.size()-1 ) );

    if ( (tr && !tr->execute(triangulator)) || !triangulator.execute() )
	return false;

    if ( !triangulation_->getConnections(-2,corner2conns_) ||
	 !triangulation_->getWeights( -2, corner2conns_, corner2weights_) ||
	 !triangulation_->getConnections(-3,corner3conns_) ||
	 !triangulation_->getWeights( -3, corner3conns_, corner3weights_) ||
	 !triangulation_->getConnections(-4,corner4conns_) ||
	 !triangulation_->getWeights( -4, corner4conns_, corner4weights_) )
    {
	return false;
    }

    return true;
}


bool TriangulationArray2DInterpol::doPrepare( int nrthreads )
{
    curnode_ = 0;
    firstthreadtestpos_ = coordlist_.size();

    for ( int idx=0; idx<nrthreads; idx++ )
	coordlist_ += Coord::udf();

    return true;
}


#define mBatchSize 1000


void TriangulationArray2DInterpol::getNextNodes( TypeSet<od_int64>& res )
{
    Threads::MutexLocker lock( curnodelock_ );
    res.erase();

    while ( curnode_<nrcells_ && res.size()<mBatchSize )
    {
	if ( nodestofill_[curnode_] && !curdefined_[curnode_] )
	    res += curnode_;

	curnode_++;
    }
}


bool TriangulationArray2DInterpol::doWork( od_int64, od_int64, int thread )
{
    TypeSet<int> neighbors;
    int dupid;
    const int testidx = firstthreadtestpos_+thread;
    TypeSet<od_int64> currenttask;
    while ( shouldContinue() )
    {
	getNextNodes( currenttask );
	if ( !currenttask.size() )
	    break;

	for ( int idx=0; idx<currenttask.size(); idx++, addToNrDone(1) )
	{
	    const od_int64 curnode = currenttask[idx];
	    const int row = curnode/nrcols_;
	    const int col = curnode%nrcols_;
	    const Coord crd(rowstep_*row, colstep_*col);

	    coordlist_[testidx] = crd;

	    if ( !triangulation_->getTriangle( testidx, dupid, neighbors ) )
		return false;

	    if ( !neighbors.size() )
		return false;

	    TypeSet<double> neighborweights;
	    if ( neighbors.size()==3 )
	    {
		float weights[3];
		interpolateOnTriangle2D( coordlist_[testidx],
		    coordlist_[neighbors[0]],
		    coordlist_[neighbors[1]],
		    coordlist_[neighbors[2]],
		    weights[0], weights[1], weights[2] );
		neighborweights += weights[0];
		neighborweights += weights[1];
		neighborweights += weights[2];
	    }
	    else if ( !triangulation_->getWeights( testidx, neighbors,
						   neighborweights) )
		return false;

	    TypeSet<int> usedneigbors;
	    TypeSet<float> usedneigborsweight_;
	    for ( int idy=0; idy<neighbors.size(); idy++ )
	    {
		const double weight = neighborweights[idy];

		if ( neighbors[idy]>=0 )
		{
		    usedneigbors += coordlistindices_[neighbors[idy]];
		    usedneigborsweight_ += weight;
		}
		else
		{
		    TypeSet<int>* conns;
		    TypeSet<double>* weights;

		    if ( neighbors[idy]==-2 )
			{ conns = &corner2conns_; weights=&corner2weights_; }
		    else if ( neighbors[idy]==-3 )
			{ conns = &corner3conns_; weights=&corner3weights_; }
		    else
			{ conns = &corner4conns_; weights=&corner4weights_; }

		    for ( int idz=0; idz<conns->size(); idz++ )
		    {
			usedneigbors += coordlistindices_[(*conns)[idz]];
			usedneigborsweight_ += (*weights)[idz] * weight;
		    }
		}
	    }

	    if ( !usedneigbors.size() )
		return false;

	    setFrom( curnode, usedneigbors.arr(), usedneigborsweight_.arr(),
		     usedneigborsweight_.size() );
	}
    }

    return true;
}
