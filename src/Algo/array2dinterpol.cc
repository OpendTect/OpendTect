/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2009
-*/

static const char* rcsID = "$Id: array2dinterpol.cc,v 1.5 2009-04-23 18:08:50 cvskris Exp $";

#include "array2dinterpolimpl.h"
#include "arrayndimpl.h"
#include "polygon.h"
#include "limits.h"
#include "rowcol.h"
#include "sorting.h"

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


bool Array2DInterpol::setArray( Array2D<float>& arr )
{
    arr_ = &arr;
    arrsetter_ = 0;
    nrrows_ = arr.info().getSize(0);
    nrcols_ = arr.info().getSize(1);
    nrcells_ = nrrows_*nrcols_;

    return true;
}


bool Array2DInterpol::setArray( ArrayAccess& arr )
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
				      bool* shouldinterpol ) const
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

	ODPolygon<float> poly;
	TypeSet<int> inside( nrcols_, -1 );
	Interval<int> rowrg, colrg;
	bool rgset = false;

	for ( int icol=0; icol<nrcols_; icol++ )
	{
	    int idx = icol;
	    for ( int irow=0; irow<nrrows_; irow++, idx+=nrcols_ )
	    {
		if ( def[idx] ) 
		{
		    poly.add( Geom::Point2D<float>(irow,icol) );
		    inside[icol] = irow;
		    mUpdateRange( irow, icol );

		    idx = (nrrows_-1)*nrcols_+icol;
		    for ( int jrow=nrrows_-1; jrow>irow; jrow--, idx-= nrcols_ )
		    {
			if ( def[idx] )
			{
			    poly.add( Geom::Point2D<float>(jrow,icol) );
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

	for ( int icol=colrg.start; icol<=colrg.stop; icol++ )
	{
	    if ( inside[icol] < 0 )
	    {
		for ( int irow=rowrg.start; irow<=rowrg.stop; irow++ )
		{
		    if ( poly.isInside( Geom::Point2D<float>(irow,icol),
					 true, 0 ) )
		    {
			inside[icol] = irow;
			break;
		    }
		}

		if ( inside[icol] < 0 )
		    continue;
	    }

	    for ( int dir=-1; dir<=1; dir+=2 )
	    {
		for ( int irow=inside[icol]; rowrg.includes(irow); irow+=dir)
		{
		    const int idx = icol+irow*nrcols_;
		    if ( def[idx] ) 
			continue;

		    if ( !poly.isInside( Geom::Point2D<float>(irow,icol),
					 true, 0 ) )
			break;

		    shouldinterpol[idx] = true;
		}
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
{}


InverseDistanceArray2DInterpol::~InverseDistanceArray2DInterpol()
{
    delete [] nodestofill_;
}


bool InverseDistanceArray2DInterpol::setArray( Array2D<float>& arr )
{
    if ( !Array2DInterpol::setArray(arr) )
	return false;

    return initFromArray();
}


bool InverseDistanceArray2DInterpol::setArray( ArrayAccess& arr )
{
    if ( !Array2DInterpol::setArray(arr) )
	return false;

    return initFromArray();
}


bool InverseDistanceArray2DInterpol::initFromArray()
{
    if ( !arr_ && !arrsetter_ )
	return false;

    nrinitialdefined_ = 0;

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

    mTryAlloc( nodestofill_, bool[nrcells_] );
    if ( !nodestofill_ )
	return false;

    getNodesToFill( curdefined_, nodestofill_ );

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
		const int coldist2 = frelcol*frelcol;
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
		const int coldist2 = coldist*coldist;
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
	      definedidxs_.size() && !stepidx_ )
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


//FillHolesArray2DInterpol
/*
#define cA2DStateDefined		0
#define cA2DStateNeedInterp		-1
#define cA2DStateMarkedForKeepUdf       -3


void FillHolesArray2DInterpol::initClass()
{ Array2DInterpol::factory().addCreator( create, sType() ); }


Array2DInterpol* FillHolesArray2DInterpol::create()
{
    return new FillHolesArray2DInterpol;
}


FillHolesArray2DInterpol::FillHolesArray2DInterpol()
    : maxnrsteps_( -1 )
    , state_( 0 )
    , nrthreadswaiting_( 0 )
{
}


FillHolesArray2DInterpol::~FillHolesArray2DInterpol()
{
    delete [] state_;
}


bool FillHolesArray2DInterpol::setArray( Array2DInterpolAccess& arr )
{
    if ( !Array2DInterpol::setArray( arr ) )
	return false;

    mTryAlloc( state_, short[nrcells_] );
    if ( !state_ )
	return false;

    mDeclareAndTryAlloc( ArrPtrMan<bool>, isdef, bool[nrcells_] );
    for ( int idx=0; idx<nrcells_; idx++ )
	isdef[idx] = !arr_->isDefined( idx );

    mDeclareAndTryAlloc( ArrPtrMan<bool>, nodestofill, bool[nrcells_] );
    if ( !nodestofill )
    {
	delete [] state_; state_ = 0;
	return false;
    }

    getNodesToFill( nodestofill, isdef );

    totalnr_ = 0;
    for ( int idx=0; idx<nrcells_; idx++ )
    {
	if ( isdef[idx] )
	    state_[idx] = cA2DStateDefined;
	else
	{
	    if ( nodestofill[idx] )
	    {
		state_[idx] = cA2DStateNeedInterp;
		totalnr_ = 0;
	    }
	    else
	    {
		state_[idx] = cA2DStateMarkedForKeepUdf;
	    }
	}
    }

    return true;
}


bool FillHolesArray2DInterpol::doPrepare( int nrthreads )
{
    if ( !state_ )
	return false;

    prepareEpoch();

    nrthreads_ = nrthreads;
    return true;
}


#define mAddNeighbor( rel, dowhat ) \
neighboridx = idx rel; \
if ( neighboridx>=0 && neighboridx<nrcells_ && state_[neighboridx]>=0 ) \
    dowhat


#define mPrepareEpochAddSource(  rel ) \
mAddNeighbor( rel, \
{ \
    minneighborstate = mMIN(state_[neighboridx],minneighborstate); \
    nrneighbors++; \
} );
void FillHolesArray2DInterpol::prepareEpoch() 
{
    curtarget_ = 0;
    epochtargets_.erase();
    unsigned char epochnrneighbors = 0;

    for ( int idx=0; idx<nrcells_; idx++ )
    {
	if ( state_[idx]!=cA2DStateNeedInterp )
	    continue;

	unsigned char nrneighbors = 0;
	const int col = idx%nrcols_;

	int neighboridx;
	short minneighborstate = SHRT_MAX;

	mPrepareEpochAddSource( -nrcols_ );
	mPrepareEpochAddSource( +nrcols_ );

	if ( col>0 )
	{
	    mPrepareEpochAddSource( -nrcols_-1 );
	    mPrepareEpochAddSource( -1 );
	    mPrepareEpochAddSource( +nrcols_-1 );
	}
	if ( col<nrcols_-1 )
	{
	    mPrepareEpochAddSource( -nrcols_+1 );
	    mPrepareEpochAddSource( +1 );
	    mPrepareEpochAddSource( +nrcols_+1 );
	}

	if ( nrneighbors )
	    continue;

	if ( maxnrsteps_>0 && minneighborstate>=maxnrsteps_ )
	    continue;

	if ( nrneighbors>epochnrneighbors )
	{
	    epochnrneighbors = nrneighbors;
	    epochtargets_.erase();
	}

	epochtargets_ += idx;
    }
}


#define mAddSource(  rel, wt ) \
mAddNeighbor( rel, \
{ \
    minneighborstate = mMIN(state_[neighboridx],minneighborstate); \
    weights[nrneighbors] = wt; \
    sources[nrneighbors]=neighboridx; \
    nrneighbors++; \
} );
bool FillHolesArray2DInterpol::doWork( od_int64 start, od_int64 stop, int )
{
    od_int64 nrleft = stop-start+1;
    float weights[8];
    int sources[8];

    const float rowfactor_ = arr_->rowFactor();
    const float diagweight = 1/Math::Sqrt( 1+rowfactor_ );

    while ( nrleft )
    {
	const od_int64 idx = getNextIdx();
	if ( idx<0 )
	    break;

	const int col = idx%nrcols_;

	unsigned char nrneighbors = 0;
	int neighboridx;
	short minneighborstate = SHRT_MAX;
	mAddSource( -nrcols_, rowfactor_ ); 
	mAddSource( +nrcols_, rowfactor_ );

	if ( col>0 )
	{
	    mAddSource( -nrcols_-1, diagweight );
	    mAddSource( -1, 1 );
	    mAddSource( +nrcols_-1, diagweight )
	}
	if ( col<nrcols_-1 )
	{
	    mAddSource( -nrcols_+1, diagweight );
	    mAddSource( +1, 1 );
	    mAddSource( +nrcols_+1, diagweight );
	}

	arr_->setPosFrom( idx, sources, weights, nrneighbors );
	reportDone( idx, minneighborstate+1 );

	nrleft--;
    }

    return true;
}


void FillHolesArray2DInterpol::reportDone( od_int64 target, short state )
{
    const int idx=epochtargets_.indexOf( target );
    if ( idx<0 )
    {
	pErrMsg( "Something fishy is happening" );
	return;
    }

    epochstates_[idx] = state;
}


od_int64 FillHolesArray2DInterpol::getNextIdx()
{
    Threads::MutexLocker lock( condvar_ );

    //Is someone else updating the epoch? If so, wait
    if ( nrthreadswaiting_ )
    {
	nrthreadswaiting_++;
	condvar_.signal( true ); //Tell epoch-updating thread that I'm here.
	while ( nrthreadswaiting_ )
	    condvar_.wait();

	if ( !epochtargets_.size() )	//There's nothing left to do
	    return -1;
    }

    if ( curtarget_<epochtargets_.size() )
    {
	const od_int64 res = epochtargets_[curtarget_];
	curtarget_++;
	return res;
    }

    //We need to wait here for every trace to finish old epoch
    nrthreadswaiting_ = 1; //Inicate that I will wait for everyone else
    while ( nrthreadswaiting_<nrthreads_ )
	condvar_.wait();

    for ( int idx=epochtargets_.size()-1; idx>=0; idx++ )
	state_[epochtargets_[idx]] = epochstates_[idx];

    prepareEpoch();

    od_int64 res = -1;
    if ( epochtargets_.size() )
    {
	res = epochtargets_[curtarget_];
	curtarget_++;
    }

    nrthreadswaiting_ = 0;
    condvar_.signal( true );

    return res;
}
*/
