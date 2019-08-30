/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2009
-*/


#include "array2dinterpolimpl.h"

#include "arrayndimpl.h"
#include "envvars.h"
#include "executor.h"
#include "delaunay.h"
#include "polygon.h"
#include "limits.h"
#include "linear.h"
#include "rowcol.h"
#include "statruncalc.h"
#include "thread.h"
#include "trckeysampling.h"
#include "trigonometry.h"

#define mPolygonType int

const char* Array2DInterpol::sKeyFillType()	{ return "Fill Type"; }
const char* Array2DInterpol::sKeyPolyNrofNodes()
{ return "Polygon nr of nodes"; }
const char* Array2DInterpol::sKeyPolyNode()	{ return "PolyNode"; }
const char* Array2DInterpol::sKeyCropPolygon()	{ return "Crop Polygon"; }
const char* Array2DInterpol::sKeyRowStep()	{ return "Row Step"; }
const char* Array2DInterpol::sKeyColStep()	{ return "Col Step"; }
const char* Array2DInterpol::sKeyOrigin()	{ return "Origin"; }
const char* Array2DInterpol::sKeyNrRows()	{ return "Nr of Rows"; }
const char* Array2DInterpol::sKeyNrCols()	{ return "Nr of Cols"; }
const char* Array2DInterpol::sKeyNrCells()	{ return "Nr of Cells"; }
const char* Array2DInterpol::sKeyMaxHoleSz()	{ return "Max Hole Size"; }

const char* InverseDistanceArray2DInterpol::sKeySearchRadius()
{ return "Search Radius"; }
const char* InverseDistanceArray2DInterpol::sKeyCornersFirst()
{ return "Corners First"; }
const char* InverseDistanceArray2DInterpol::sKeyStepSize()
{ return "Step Size"; }
const char* InverseDistanceArray2DInterpol::sKeyNrSteps()
{ return "Nr of Steps"; }

const char* TriangulationArray2DInterpol::sKeyDoInterpol()
{ return "Do Interpolation"; }
const char* TriangulationArray2DInterpol::sKeyMaxDistance()
{ return "Maximum Distance"; }

const char* ExtensionArray2DInterpol::sKeyNrSteps()
{ return "Nr of Steps"; }



mDefineEnumUtils( Array2DInterpol, FillType, "Filltypes" )
{ "Only Holes", "Convex Hull", "Full", 0 };

template<>
void EnumDefImpl<Array2DInterpol::FillType>::init()
{
    uistrings_ += mEnumTr("Only Holes",0);
    uistrings_ += mEnumTr("Convex Hull",0);
    uistrings_ += uiStrings::sFull();
}

mImplClassFactory( Array2DInterpol, factory );

class Extension2DInterpolExecutor : public Executor
{ mODTextTranslationClass(Extension2DInterpolExecutor);
public:
		Extension2DInterpolExecutor(ExtensionArray2DInterpol&);
		~Extension2DInterpolExecutor()  { deleteStateArr(); }

    int		nextStep();
    uiString	message() const	{ return curmsg_; }
    od_int64	nrDone() const		{ return curlvl_ + 1; }
    uiString	nrDoneText() const	{ return tr("Interpolation level"); }
    od_int64	totalNr() const		{ return aie_.getNrSteps(); }

protected:

    void	createStateArr();
    void	deleteStateArr();
    void	adjustInitialStates();
    void	excludeBigHoles();
    bool	markBigHoles();
    void	floodFill4KeepUdf(int,int);
    void	handleAdjCol(int,int,int);
    bool	doInterpolate(int,int);
    bool	interpExtension(int,int,float&);

    short**	state_;
    int		curlvl_;
    float	diagdist_;
    uiString	curmsg_;

    ExtensionArray2DInterpol&	aie_;
};


Array2DInterpol::Array2DInterpol()
    : arr_(0)
    , arrsetter_(0)
    , nrcells_(-1)
    , nrrows_(-1)
    , nrcols_(-1)
    , filltype_(Full)
    , maxholesize_(mUdf(float))
    , rowstep_(1)
    , colstep_(1)
    , origin_(-1,-1)
    , mask_(0)
    , maskismine_(false)
    , isclassification_(false)
    , statsetup_(0)
    , trend_(0)
    , poly_(0)
    , croppoly_(false)
{}


Array2DInterpol::~Array2DInterpol()
{
    if ( maskismine_ ) delete mask_;
    delete statsetup_;
    delete trend_;
    delete poly_;
}


void Array2DInterpol::setClassification( bool yn )
{ isclassification_ = yn; }


bool Array2DInterpol::isClassification() const
{ return isclassification_; }


void Array2DInterpol::setFillType( FillType ft )
{ filltype_ = ft; arr_ = 0; arrsetter_=0; }


void Array2DInterpol::setRowStep( float rs )
{ rowstep_ = rs; arr_ = 0; arrsetter_ = 0; }


void Array2DInterpol::setColStep( float cs )
{ colstep_ = cs; arr_ = 0; arrsetter_ = 0; }

void Array2DInterpol::setOrigin( const RowCol& rc )
{ origin_ = rc; }


void Array2DInterpol::setTrendOrder( PolyTrend::Order ord )
{
    delete trend_;
    trend_ = new PolyTrend();
    trend_->setOrder( ord );
}


void Array2DInterpol::setSampling( const TrcKeySampling& hs )
{
    setRowStep( (float)hs.lineDistance() );
    setColStep( (float)hs.trcDistance() );
    setOrigin( hs.start_ );
}

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


bool Array2DInterpol::setArray( Array2D<float>& arr, const TaskRunnerProvider& )
{
    arr_ = &arr;
    arrsetter_ = 0;
    nrrows_ = arr.getSize(0);
    nrcols_ = arr.getSize(1);
    nrcells_ = nrrows_*nrcols_;

    return true;
}


bool Array2DInterpol::setArray( ArrayAccess& arr, const TaskRunnerProvider& )
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


void Array2DInterpol::getNodesToFill( const bool* def, bool* shouldinterpol,
				      const TaskRunnerProvider& ) const
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

    const bool defval = filltype_==Polygon ? false : filltype_!=ConvexHull;
    if ( defval == false )
	OD::memZero( shouldinterpol, nrcells_ );
    else
	OD::memValueSet( shouldinterpol, defval, nrcells_ );

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
	    pt.y_ = icol;
	    int irow = pt.x_ = prevstart;
	    if ( icol!=colrg.start && !poly.isInside( pt, true, 0 ) )
		irow = pt.x_ = rowrg.start;

	    bool hadaninside = false;
	    bool isinside;
	    while ( true )
	    {
		isinside = poly.isInside( pt, true, 0 );
		if ( !isinside )
		    break;

		hadaninside = true;

		if ( !rowrg.includes(irow,false) ) break;

		irow--;
		pt.x_ = irow;
	    }

	    if ( !hadaninside || !isinside )
		irow++;

	    if ( !hadaninside )
	    {
		pt.x_ = irow;
		while ( !poly.isInside( pt, true, 0 ) )
		{
		    irow++;
		    pt.x_ = irow;
		}
	    }

	    Interval<int> thisrowrg;
	    thisrowrg.start = prevstart = irow;
	    irow = prevstop;
	    pt.x_ = irow;
	    hadaninside = false;

	    while ( true )
	    {
		isinside = poly.isInside( pt, true, 0 );
		if ( !isinside )
		    break;

		hadaninside = true;

		if ( !rowrg.includes(irow,false) ) break;

		irow++;
		pt.x_ = irow;
	    }

	    if ( !hadaninside || !isinside )
		irow--;

	    if ( !hadaninside )
	    {
		pt.x_ = irow;
		while ( !poly.isInside( pt, true, 0 ) )
		{
		    irow--;
		    pt.x_ = irow;
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
    else if ( filltype_==Polygon )
    {
	if ( !poly_ )
	    return;

	for ( int idx=0; idx<nrcells_; idx++ )
	{
	    if ( def[idx] ) continue;

	    const double xpos = mCast(double,idx / nrcols_ + origin_.inl());
	    const double ypos = mCast(double,idx % nrcols_ + origin_.crl());
	    if ( poly_->isInside(Geom::Point2D<double>(xpos,ypos),true,0) )
		shouldinterpol[idx] = true;
	}
    }

    excludeBigHoles( def, shouldinterpol );

    if ( !mask_ )
	return;

    if ( mask_->getSize(0)==nrrows_ &&
	 mask_->getSize(1)==nrcols_ &&
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
	const int masknrrows = mMIN(nrrows_,mask_->getSize(0) );
	const int masknrcols = mMIN(nrcols_,mask_->getSize(1) );
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
    calc.addValue( val, weights[idx] ); \
}


bool Array2DInterpol::doPrepare( int )
{
    if ( arrsetter_ )
	return true;

    delete statsetup_;
    mTryAlloc( statsetup_, Stats::CalcSetup( true ) );
    if ( !statsetup_ )
	return false;

    if ( isclassification_ )
	statsetup_->require( Stats::MostFreq );
    else
	statsetup_->require( Stats::Average );

    return true;
}


void Array2DInterpol::setFrom( od_int64 target, const od_int64* sources,
			       const float* weights, int nrsrc)
{
    if ( !nrsrc )
	return;

    if ( arrsetter_ )
    {
	arrsetter_->set( target, sources, weights, nrsrc, isclassification_ );
	return;
    }

    Stats::RunCalc<double> calc( *statsetup_ );

    float* ptr = arr_->getData();
    if ( ptr )
    {
	mDoLoop( const float val = ptr[sources[idx]] );
	ptr[target] = isclassification_
	    ? (float) calc.mostFreq()
	    : (float) calc.average();
    }
    else
    {
	ValueSeries<float>* storage = arr_->getStorage();
	if ( storage )
	{
	    mDoLoop( const float val = storage->value(sources[idx]) );
	    storage->setValue(target,
		isclassification_
		    ? (float) calc.mostFreq()
		    : (float) calc.average());
	}
	else
	{
	    mDoLoop( const od_int64 src = sources[idx];
	        const float val =
		  arr_->get(mCast(int,src/nrcols_),mCast(int,src%nrcols_) ) );
	    arr_->set( mCast(int,target/nrcols_), mCast(int,target%nrcols_),
		isclassification_
		? (float) calc.mostFreq()
		: (float) calc.average());
	}
    }
}


void Array2DInterpol::floodFillArrFrom( int seed, const bool* def,
					bool* shouldinterpol ) const
{
    shouldinterpol[seed] = false;

    int next;

    TypeSet<int> seeds( seed );
    while ( seeds.size() )
    {
	const int curseedidx = seeds.size()-1;
	const int curseed = seeds[curseedidx];
	seeds.removeSingle( curseedidx, false );
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


bool Array2DInterpol::fillPar( IOPar& par ) const
{
    par.set( sKeyFillType(), filltype_ );
    if ( filltype_==Polygon )
    {
	if ( !poly_ ) return false;

	par.setYN( sKeyCropPolygon(), croppoly_ );
	par.set( sKeyPolyNrofNodes(), poly_->size() );
	for ( int idx=0; idx<poly_->size(); idx++ )
	{
	    Coord node( poly_->getVertex(idx) );
	    par.set( IOPar::compKey(sKeyPolyNode(),idx), node );
	}
    }

    par.set( sKeyRowStep(), rowstep_ );
    par.set( sKeyColStep(), colstep_ );
    par.set( sKeyOrigin(), origin_.row(), origin_.col() );
    par.set( sKeyNrRows(), nrrows_ );
    par.set( sKeyNrCols(), nrcols_ );
    par.set( sKeyNrCells(), nrcells_ );
    par.set( sKeyMaxHoleSz(), mIsUdf(maxholesize_) ? -1 : maxholesize_ );
    if ( trend_ )
	par.set( PolyTrend::sKeyOrder(),
		 PolyTrend::OrderDef().getKey(trend_->getOrder()) );
    return true;
}


bool Array2DInterpol::usePar( const IOPar& par )
{
    int filltype;
    par.get( sKeyFillType(), filltype );
    filltype_ = (Array2DInterpol::FillType)filltype;
    if ( filltype_==Polygon )
    {
	delete poly_; poly_ = 0;

	par.getYN( sKeyCropPolygon(), croppoly_ );
	int nrnodes = 0;
	par.get( sKeyPolyNrofNodes(), nrnodes );
	if ( nrnodes>0 )
	{
	    poly_ = new ODPolygon<double>();
	    for ( int idx=0; idx<nrnodes; idx++ )
	    {
		Coord node;
		if ( par.get( IOPar::compKey(sKeyPolyNode(),idx), node ) )
		    poly_->add( node );
	    }
	    poly_->setClosed( true );
	}
    }

    par.get( sKeyRowStep(), rowstep_ );
    par.get( sKeyColStep(), colstep_ );
    par.get( sKeyOrigin(), origin_.row(), origin_.col() );
    par.get( sKeyNrRows(), nrrows_ );
    par.get( sKeyNrCols(), nrcols_ );
    par.get( sKeyNrCells(), nrcells_ );
    par.get( sKeyMaxHoleSz(), maxholesize_ );

    FixedString trendstr = par.find( PolyTrend::sKeyOrder() );
    if ( !trendstr.isEmpty() )
	setTrendOrder( PolyTrend::OrderDef().parse(trendstr) );

    return true;
}

bool Array2DInterpol::trimArray( int step, Array2D<char>& edgesmask )
{
    if ( !arr_ || mIsUdf(step) || step < 1 || !edgesmask.isOK() ||
	 nrrows_ < 2 || nrcols_ < 2 )
	return false;

    edgesmask.setAll( '\0' );

    const int n1m = nrrows_-1;
    const int n2m = nrcols_-1;

    for ( int idx=0; idx<nrrows_; idx++ )
    {
	for ( int idy=0; idy<nrcols_; idy++ )
	{
	    int requestnbneighbors = idx < 1 || idx > n1m ? 1 : 2;
	    requestnbneighbors += idy < 1 || idy > n2m ? 1 : 2;
	    if ( mIsUdf(arr_->get(idx,idy)) )
		continue;

	    int nbneighbors = 0;
	    if ( idx > 0 && !mIsUdf(arr_->get(idx-1,idy)) )
		nbneighbors++;
	    if ( idx < n1m && !mIsUdf(arr_->get(idx+1,idy)) )
		nbneighbors++;
	    if ( idy > 0 && !mIsUdf(arr_->get(idx,idy-1)) )
		nbneighbors++;
	    if ( idy < n2m && !mIsUdf(arr_->get(idx,idy+1)) )
		nbneighbors++;

	    if ( (nbneighbors < requestnbneighbors) ||
		 (idx%step == 0 && idy%step == 0) )
	    {
		edgesmask.set( idx, idy, '\1' );
	    }
	}
    }

    return true;
}


mDefParallelCalc5Pars( ArrPolyCropper, od_static_tr("ArrPolyCropper",
	    "Crop array along polygon"),
	const ODPolygon<double>*, poly, Array2D<float>&, arr, int, nrrows,
	int, nrcols, const RowCol&, origin )
mDefParallelCalcBody
(
	const Array2DInfoImpl info2d( nrrows_, nrcols_ );
	ArrayNDIter iter( info2d );
	iter.setGlobalPos( start );
    ,
	const int iidx = iter[0];
	const int iidy = iter[1];
	Geom::Point2D<double> pos( mCast(double,iidx + origin_.row()),
				   mCast(double,iidy + origin_.col()) );
	if ( !poly_->isInside(pos,true,0) )
	    arr_.set( iidx, iidy, mUdf(float) );

	iter.next();
    ,
)


bool Array2DInterpol::doFinish( bool success )
{
    if ( filltype_==Polygon && poly_ && croppoly_ && arr_ )
    {
	//arrsetter_ is not considered since its never been finished
	ArrPolyCropper polycrop( nrcells_, poly_, *arr_, nrrows_, nrcols_,
				 origin_ );
	polycrop.execute();
    }

    return success;
}


//InverseDistance
//
//
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
    , condvar_( *new Threads::ConditionVar )
    , totalnr_( -1 )
    , nrthreads_( mUdf(int) )
    , nraddedthisstep_( mUdf(int) )
    , prevsupportsize_( mUdf(int) )
{}


InverseDistanceArray2DInterpol::~InverseDistanceArray2DInterpol()
{
    delete [] nodestofill_;
    delete [] curdefined_;
    delete &condvar_;
}


bool InverseDistanceArray2DInterpol::setArray( Array2D<float>& arr,
					   const TaskRunnerProvider& trprov )
{
    if ( !Array2DInterpol::setArray(arr,trprov) )
	return false;

    return initFromArray( trprov );
}


bool InverseDistanceArray2DInterpol::setArray( ArrayAccess& arr,
					   const TaskRunnerProvider& trprov )
{
    if ( !Array2DInterpol::setArray(arr, trprov ) )
	return false;

    return initFromArray( trprov );
}


bool InverseDistanceArray2DInterpol::initFromArray(
				const TaskRunnerProvider& trprov )
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

    getNodesToFill( curdefined_, nodestofill_, trprov );

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
    if ( !Array2DInterpol::doPrepare(nrthreads) )
	return false;

    nrthreads_ = nrthreads;
    if ( !nrinitialdefined_ )
	return false; //Nothing defined;

    definedidxs_.setEmpty();
    neighbors_.setEmpty();
    neighborweights_.setEmpty();
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
	const int rowradius = mNINT32( searchradius_/rowstep_ );
	const int colradius = mNINT32( searchradius_/colstep_ );
	const float radius2 = searchradius_*searchradius_;

	for ( int relrow=0; relrow<=rowradius; relrow++ )
	{
	    const float frelrow = relrow*rowstep_;
	    const float rowdist2 = frelrow*frelrow;

	    for ( int relcol=0; relcol<=colradius; relcol++ )
	    {
		if ( !relrow && !relcol)
		    continue;

		const float frelcol = relcol*colstep_;
		const int coldist2 = mNINT32(frelcol*frelcol);
		const float dist2 = coldist2+rowdist2;
		if ( dist2>radius2 )
		    continue;

		const float dist = Math::Sqrt( dist2 );
		float weight = ( searchradius_ - dist )/( searchradius_*dist );
		weight *= weight;

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
    ArrPtrMan<od_int64> sources = 0;

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
	mTryAllocPtrMan( sources, od_int64[maxnr] );
	if ( !weights )
	    return false;
    }

    while ( true )
    {
	const od_int64 idx = getNextIdx();
	if ( idx<0 )
	    break;

	int targetrow = (int) idx/nrcols_, targetcol = (int) idx%nrcols_;

	if ( definedidxs_.size() ) //No search radius, do all pts
	{
	    for ( int idy=definedidxs_.size()-1; idy>=0; idy-- )
	    {
		const od_int64 source = definedidxs_[idy];
		if ( source == idx )
		{
		    // We are at a data point, so other sources are discarded.
		    for ( int widx=0; widx<definedidxs_.size(); widx++ )
			weights[widx] = 0;
		    weights[idy] = 1;
		    break;
		}

		const float sourcerow = mCast(float,source/nrcols_);
		const float sourcecol = mCast(float,source%nrcols_);

		const float rowdist = (targetrow-sourcerow)*rowstep_;
		const float rowdist2 = rowdist*rowdist;
		const float coldist = (targetcol-sourcecol)*colstep_;
		const int coldist2 = mNINT32(coldist*coldist);
		const float weight = 1.f/( coldist2+rowdist2 );

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
		const int sourcerow = targetrow+rc.row();
		if ( sourcerow<0 || sourcerow>=nrrows_ )
		    continue;

		const int sourcecol = targetcol+rc.col();
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


bool InverseDistanceArray2DInterpol::fillPar( IOPar& par ) const
{
    Array2DInterpol::fillPar( par );
    par.set( sKeySearchRadius(), searchradius_ );
    par.set( sKeyStepSize(), stepsize_ );
    par.set( sKeyNrSteps(), nrsteps_ );
    par.setYN( sKeyCornersFirst(), cornersfirst_ );
    return true;
}

bool InverseDistanceArray2DInterpol::usePar( const IOPar& par )
{
    Array2DInterpol::usePar( par );
    par.get( sKeySearchRadius(), searchradius_ );
    par.get( sKeyStepSize(), stepsize_ );
    par.get( sKeyNrSteps(), nrsteps_ );
    par.getYN( sKeyCornersFirst(), cornersfirst_ );
    return true;
}


class InvDistArr2DGridFindSources : public ParallelTask
{
public:
    InvDistArr2DGridFindSources( const bool* def, const bool* nodestofill,
				 int nrrows, int nrcols,
				 TypeSet<od_int64>& res,
				 TypeSet<od_int64>& nrs,
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
	TypeSet<od_int64> localres;
	TypeSet<od_int64> nrsourceslist;

	for ( od_int64 idx=start; idx<=stop; idx++ )
	{
	    if ( curdefined_[idx] || !nodestofill_[idx] )
		continue;

	    const int col = mCast(int,idx%nrcols_);
	    const int row = mCast(int,idx/nrcols_);

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
		    const od_int64 offset = idx+idy+idz*nrcols_;

		    if ( offset<0 )
		    {
			const int targetrow = row+idz;
			idz -= targetrow+1;
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

	Threads::Locker lckr( lock_ );
	if ( cornersfirst_ )
	{
	    if ( maxnrsources<maxnrsources_ )
		return true;

	    if ( maxnrsources>maxnrsources_ )
	    {
		res_.erase();
		nrsourceslist_.erase();
		maxnrsources_ = maxnrsources;
	    }
	}

	nrsourceslist_.append( nrsourceslist );
	res_.append( localres );
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
    TypeSet<od_int64>&		res_;
    TypeSet<od_int64>&		nrsourceslist_;
    int				stepsize_;
    int				maxnrsources_;
    bool			cornersfirst_;

    Threads::Lock		lock_;
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
    Threads::MutexLocker lckr( condvar_ );

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
    const od_int64 res = todothisstep_[nrleft-1];
    todothisstep_.removeSingle( nrleft-1 );
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
	nrsources_.removeSingle( nrleft-1 );
    }

    mRet( res );
}

#undef mRet


void InverseDistanceArray2DInterpol::reportDone( od_int64 idx )
{
    addToNrDone( 1 );
    Threads::MutexLocker lckr( condvar_ );
    addedwithcursuport_ += idx;
    nraddedthisstep_++;
}


// Triangulation

TriangulationArray2DInterpol::TriangulationArray2DInterpol()
    : triangulation_(0)
    , triangleinterpolator_(0)
    , curdefined_(0)
    , nodestofill_(0)
    , totalnr_(-1)
    , dointerpolation_(true)
    , maxdistance_(mUdf(float))
{}


TriangulationArray2DInterpol::~TriangulationArray2DInterpol()
{
    delete [] nodestofill_;
    delete [] curdefined_;
    delete triangleinterpolator_;
}


bool TriangulationArray2DInterpol::setArray( Array2D<float>& arr,
					   const TaskRunnerProvider& trprov )
{
    if ( !Array2DInterpol::setArray(arr,trprov) )
	return false;

    return initFromArray( trprov );
}


bool TriangulationArray2DInterpol::setArray( ArrayAccess& arr,
					     const TaskRunnerProvider& trprov )
{
    if ( !Array2DInterpol::setArray(arr,trprov) )
	return false;

    return initFromArray( trprov );
}


bool TriangulationArray2DInterpol::initFromArray(
					    const TaskRunnerProvider& trprov )
{
    if ( !arr_ && !arrsetter_ )
	return false;

    delete [] curdefined_;
    mTryAlloc( curdefined_, bool[nrcells_] );
    if ( !curdefined_ )
	return false;

    for ( int idx=0; idx<nrcells_; idx++ )
	curdefined_[idx] = isDefined( idx );

    delete [] nodestofill_;
    mTryAlloc( nodestofill_, bool[nrcells_] );
    if ( !nodestofill_ )
	return false;

    getNodesToFill( curdefined_, nodestofill_, trprov );

    totalnr_ = 0;
    const bool* ptr = curdefined_;
    const bool* stopptr = curdefined_+nrcells_;
    const bool* nodestofillptr = nodestofill_;

    while ( ptr!=stopptr )
    {
	if ( *nodestofillptr && !*ptr )
	    totalnr_++;

	ptr++;
	nodestofillptr++;
    }

    if ( !totalnr_ )
	return true;

    //Trend settings
    setTrendOrder( PolyTrend::Order0 );
    Array2DImpl<char> edgesmask( arr_->info() );
    const bool usetrimming = trend_ ? trimArray( 8, edgesmask ) : false;
    //Get defined nodes to triangulate
    TypeSet<Coord>* coordlist = new TypeSet<Coord>;
    coordlistindices_.erase();
    ptr = curdefined_;

    TypeSet<Coord> trendpts;
    TypeSet<float> trendvals;
    TypeSet<Coord3> horpts;

    double avgz = 0.;
    od_int64 nrdef = 0;

    bool hascorner[4];
    for ( int idx=0; idx<4; idx++ )
    {
	hascorner[idx ] = false;
	corneridx_[idx] = -(idx+1);
	cornerval_[idx] = 0.f;
	cornerbid_[idx] = BinID( 0, 0 );
	if ( idx>1 )
	    cornerbid_[idx].inl() = nrrows_-1;
	if ( idx==1 || idx==2 )
	    cornerbid_[idx].crl() = nrcols_-1;
    }

    int curidx = 0;
    while ( ptr!=stopptr )
    {
	if ( *ptr )
	{
	    int pos[2];
	    arr_->info().getArrayPos( curidx, pos );
	    const float val = arr_->get( pos[0], pos[1] );

	    if ( !usetrimming || edgesmask.get(pos[0],pos[1])=='\1' )
	    {
		trendpts += Coord( pos[0], pos[1] );
		trendvals += val;

		(*coordlist) += Coord(pos[0], pos[1]);
		coordlistindices_ += curidx;
	    }
	    else if ( pos[0]%4==0 && pos[1]%4==0 )
	    {
		(*coordlist) += Coord(pos[0], pos[1]);
		coordlistindices_ += curidx;
	    }

	    horpts += Coord3( pos[0], pos[1], val );
	    avgz += val;
	    nrdef++;

	    for ( int cidx=0; cidx<4; cidx++ )
	    {
		if ( cornerbid_[cidx]==BinID(pos[0],pos[1]) )
		{
		    hascorner[cidx] = true;
		    cornerval_[cidx] = val;
		}
	    }
	}

	curidx++;
	ptr++;
    }

    if ( coordlist->isEmpty() )
	return false;

    const bool usefitplane = GetEnvVar("USE_BESTFIT_PLANE");
    Plane3DFit planefit;
    Plane3 bestfitplane;
    if ( usefitplane )
	planefit.compute(horpts,bestfitplane);

    for ( int idx=0; idx<4; idx++ )
    {
	if ( hascorner[idx] )
	    continue;

	const Coord pos( cornerbid_[idx].inl(), cornerbid_[idx].crl() );
	(*coordlist) += pos;
	coordlistindices_ += corneridx_[idx];
	if ( usefitplane && !mIsZero(bestfitplane.C_,1e-8) )
	{
	    cornerval_[idx] = mCast(float, -(bestfitplane.A_*pos.x_ +
			bestfitplane.B_*pos.y_ +
			bestfitplane.D_)/bestfitplane.C_);
	}
	else
	    cornerval_[idx] =  mCast(float,avgz/nrdef);

	trendpts += pos;
	trendvals += cornerval_[idx];
    }

    trend_->set( trendpts, trendvals.arr() );

    if ( triangulation_ )
	delete triangulation_;

    triangulation_ = new DAGTriangleTree;
    if ( !triangulation_ ||
	 !triangulation_->setCoordList( coordlist, OD::TakeOverPtr ) )
	return false;

    DelaunayTriangulator triangulator( *triangulation_ );
    triangulator.dataIsRandom( false );

    if ( !trprov.execute( triangulator ) )
	return false;

    if ( triangleinterpolator_ )
	delete triangleinterpolator_;

    triangleinterpolator_ = new Triangle2DInterpolator( *triangulation_ );

    return true;
}


bool TriangulationArray2DInterpol::doPrepare( int nrthreads )
{
    if ( !Array2DInterpol::doPrepare(nrthreads) )
	return false;

    curnode_ = 0;
    return true;
}


#define mBatchSize 1000


void TriangulationArray2DInterpol::getNextNodes( TypeSet<od_int64>& res )
{
    Threads::Locker lckr( curnodelock_ );
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
    if ( !triangleinterpolator_ )
	return false;

    TypeSet<od_int64> currenttask;
    TypeSet<od_int64> definedidices;

    const double curmaxdist = mIsUdf(maxdistance_) ? maxdistance_ :
	maxdistance_/mMAX(rowstep_,colstep_);
    float* data = arr_->getData();
    while ( shouldContinue() )
    {
	getNextNodes( currenttask );
	if ( !currenttask.size() )
	    break;

	for ( int idx=0; idx<currenttask.size(); idx++, addToNrDone(1) )
	{
	    const od_int64 curnode = currenttask[idx];
	    int pos[2];
	    if ( !arr_->info().getArrayPos( curnode, pos ) )
		continue;

	    const Coord crd(pos[0], pos[1]);
	    TypeSet<int> vertices;
	    TypeSet<double> weightsd;
	    if ( !triangleinterpolator_->computeWeights( crd, vertices,
			weightsd, curmaxdist, dointerpolation_ ) )
		return false;

	    const int sz = vertices.size();
	    if ( !sz ) continue;

	    if ( trend_ )
	    {
		double val = 0;
		for ( int widx=0; widx<weightsd.size(); widx++ )
		{
		    const od_int64 posidx = coordlistindices_[vertices[widx]];
		    float posval = mUdf(float);
		    Coord curpos;

		    if ( posidx>=0 )
		    {
			posval = data[posidx];
			int rc[2];
			if ( !arr_->info().getArrayPos(curnode,rc) )
			    continue;

			curpos = Coord( rc[0], rc[1] );
		    }
		    else
		    {
			for (int cidx=0; cidx<4; cidx++ )
			{
			    if ( corneridx_[cidx]==posidx )
			    {
				posval = cornerval_[cidx];
				curpos = Coord( cornerbid_[cidx].inl(),
						cornerbid_[cidx].crl() );
				break;
			    }
			}
		    }

		    if ( mIsUdf(posval) ) continue;

		    trend_->apply( curpos, true, posval );
		    val += weightsd[widx] * posval;
		}

		trend_->apply( Coord(pos[0],pos[1]), false, val );
		arr_->set( pos[0], pos[1], mCast(float,val) );
	    }
	    else
	    {
		TypeSet<float> weights;
		for ( int widx=0; widx<weightsd.size(); widx++ )
		    weights += mCast(float,weightsd[widx]);

		TypeSet<od_int64> usedindices;
		for ( int vidx=0; vidx<sz; vidx++ )
		    usedindices += coordlistindices_[vertices[vidx]];

		setFrom( curnode, usedindices.arr(), weights.arr(), sz );
	    }
	}
    }

    return true;
}


bool TriangulationArray2DInterpol::fillPar( IOPar& par ) const
{
    Array2DInterpol::fillPar( par );
    par.setYN( sKeyDoInterpol(), dointerpolation_ );
    par.set( sKeyMaxDistance(), maxdistance_ );
    return true;
}

bool TriangulationArray2DInterpol::usePar( const IOPar& par )
{
    Array2DInterpol::usePar( par );
    par.getYN( sKeyDoInterpol(), dointerpolation_ );
    par.get( sKeyMaxDistance(), maxdistance_ );
    return true;
}

// Extension
#define cA2DStateMarkedForKeepUdf       -3
#define cA2DStateKeepUdf                -2
#define cA2DStateNeedInterp             -1
#define cA2DStateDefined                0
//!< States higher than 0 mean the node was interpolated
//!< The higher the state, the further the node is away from 'defined space'


ExtensionArray2DInterpol::ExtensionArray2DInterpol()
    : executor_( 0 )
    , nrsteps_( 0 )
{}


ExtensionArray2DInterpol::~ExtensionArray2DInterpol()
{
    delete executor_;
}


bool ExtensionArray2DInterpol::doWork( od_int64, od_int64, int thread )
{
    if ( !executor_ )
	executor_ = new Extension2DInterpolExecutor( *this );

    return executor_->execute();
}


bool ExtensionArray2DInterpol::fillPar( IOPar& par ) const
{
    Array2DInterpol::fillPar( par );
    par.set( sKeyNrSteps(), nrsteps_ );
    return true;
}


bool ExtensionArray2DInterpol::usePar( const IOPar& par )
{
    Array2DInterpol::usePar( par );
    par.get( sKeyNrSteps(), nrsteps_ );
    return true;
}


Extension2DInterpolExecutor::Extension2DInterpolExecutor(
						ExtensionArray2DInterpol& aie )
    : Executor("2D Interpolation")
    , aie_(aie)
    , state_(0)
    , curlvl_(-1)
    , diagdist_(mUdf(float))
    , curmsg_(tr("Setting up interpolation"))
{
    createStateArr();
}


void Extension2DInterpolExecutor::createStateArr()
{
    state_ = new short* [aie_.nrrows_];
    for ( int idx=0; idx<aie_.nrrows_; idx++ )
    {
	state_[idx] = new short [aie_.nrcols_];
	if ( !state_[idx] )
	    deleteStateArr();
    }

    for ( int irow=0; irow<aie_.nrrows_; irow++ )
    {
	for ( int icol=0; icol<aie_.nrcols_; icol++ )
	{
	    const float val = aie_.arr_->get( irow, icol );
	    const bool isudf = mIsUdf(val);
	    state_[irow][icol] = isudf ? cA2DStateNeedInterp : cA2DStateDefined;
	}
    }
}


void Extension2DInterpolExecutor::deleteStateArr()
{
    if ( !state_ ) return;

    for ( int idx=0; idx<aie_.nrrows_; idx++ )
	delete [] state_[idx];

    delete [] state_;
    state_ = 0;
}


bool Extension2DInterpolExecutor::doInterpolate( int irow, int icol )
{
    float val;
    if ( interpExtension(irow,icol,val) )
    {
	state_[irow][icol] = mCast(short,curlvl_ + 1);
	aie_.arr_->set( irow, icol, val );
	return true;
    }

    return false;
}


#define mA2DIsValidInterpPoint(ir,ic) \
    (state_[ir][ic] >= cA2DStateDefined && state_[ir][ic] <= curlvl_)

#define mA2DInterpAdd(ir,ic,d) \
{ \
    if ( mA2DIsValidInterpPoint(ir,ic) ) \
    { \
	defs[nrdefs] = aie_.arr_->get(ir,ic); \
	wts[nrdefs] = d; \
	nrdefs++; \
    } \
}


bool Extension2DInterpolExecutor::interpExtension( int irow, int icol,
						   float& val )
{
    float defs[12]; float wts[12]; int nrdefs = 0;
    if ( irow )
    {
	if ( irow-1 && mA2DIsValidInterpPoint(irow-1,icol) )
	    mA2DInterpAdd( irow-2, icol, 2*aie_.rowstep_ );
	mA2DInterpAdd( irow-1, icol, aie_.rowstep_ );
	if ( icol )
	    mA2DInterpAdd( irow-1, icol-1, diagdist_ );
	if ( icol < aie_.nrcols_-1 )
	    mA2DInterpAdd( irow-1, icol+1, diagdist_ );
    }

    if ( icol )
    {
	if ( icol-1 && mA2DIsValidInterpPoint(irow,icol-1) )
	    mA2DInterpAdd( irow, icol-2, 2 * aie_.colstep_ );
	mA2DInterpAdd( irow, icol-1, aie_.colstep_ );
	if ( irow < aie_.nrrows_-1 )
	    mA2DInterpAdd( irow+1, icol-1, diagdist_ );
    }

    if ( irow < aie_.nrrows_-1 )
    {
	mA2DInterpAdd( irow+1, icol, aie_.rowstep_ );
	if ( icol < aie_.nrcols_-1 )
	    mA2DInterpAdd( irow+1, icol+1, diagdist_ );
	if ( irow < aie_.nrrows_-2 && mA2DIsValidInterpPoint(irow+1,icol) )
	    mA2DInterpAdd( irow+2, icol, 2 * aie_.rowstep_ );
    }

    if ( icol < aie_.nrcols_-1 )
    {
	mA2DInterpAdd( irow, icol+1, aie_.colstep_ );
	if ( icol < aie_.nrcols_-2 && mA2DIsValidInterpPoint(irow,icol+1) )
	    mA2DInterpAdd( irow, icol+2, 2 * aie_.colstep_ );
    }

    if ( nrdefs < 2 )
	return false;

    float sumval = 0, sumwt = 0;
    for ( int idx=0; idx<nrdefs; idx++ )
    {
	float wt = 1.f / wts[idx];
	sumval += defs[idx] * wt;
	sumwt += wt;
    }

    val = sumval / sumwt;
    return true;
}


void Extension2DInterpolExecutor::adjustInitialStates()
{
    if ( aie_.filltype_ != Array2DInterpol::Full )
    {
	for ( int icol=0; icol<aie_.nrcols_; icol++ )
	{
	    if ( state_[0][icol] == cA2DStateNeedInterp )
		floodFill4KeepUdf( 0, icol );
	    if ( state_[aie_.nrrows_-1][icol] == cA2DStateNeedInterp )
		floodFill4KeepUdf( aie_.nrrows_-1, icol );
	}

	for ( int irow=1; irow<aie_.nrrows_-1; irow++ )
	{
	    if ( state_[irow][0] == cA2DStateNeedInterp )
		floodFill4KeepUdf( irow, 0 );
	    if ( state_[irow][aie_.nrcols_-1] == cA2DStateNeedInterp )
		floodFill4KeepUdf( irow, aie_.nrcols_-1 );
	}
    }

#define mGeomPoint( i, j ) Geom::Point2D<float>((float) i,(float) j)

    if ( aie_.filltype_ == Array2DInterpol::ConvexHull )
    {
	ODPolygon<float> poly;
	TypeSet<int> inside( aie_.nrcols_, -1 );
	for ( int icol=0; icol<aie_.nrcols_; icol++ )
	{
	    for ( int irow=0; irow<aie_.nrrows_; irow++ )
	    {
		if ( state_[irow][icol] == cA2DStateDefined )
		{
		    poly.add( mGeomPoint(irow,icol) );
		    inside[icol] = irow;

		    for ( int jrow=aie_.nrrows_-1; jrow>irow; jrow-- )
		    {
			if ( state_[jrow][icol] == cA2DStateDefined )
			{
			    poly.add( mGeomPoint(jrow,icol) );
			    break;
			}
		    }
		    break;
		}
	    }
	}

	poly.convexHull();

	for ( int icol=0; icol<aie_.nrcols_; icol++ )
	{
	    if ( inside[icol] < 0 )
		continue;

	    for ( int dir=-1; dir<=1; dir+=2 )
	    {
		for ( int irow=inside[icol]; irow>=0 && irow<aie_.nrrows_;
			irow+=dir)
		{
		    if ( state_[irow][icol] == cA2DStateKeepUdf )
		    {
			if ( !poly.isInside( mGeomPoint(irow,icol),
				    true, 0 ) )
			    break;

			state_[irow][icol] = cA2DStateNeedInterp;
		    }
		}
	    }
	}
    }
}


bool Extension2DInterpolExecutor::markBigHoles()
{
    bool havebighole = false;
    for ( int irow=0; irow<aie_.nrrows_; irow++ )
    {
	int nrtobeinterp = 0;
	for ( int icol=0; icol<aie_.nrcols_; icol++ )
	{
	    if ( state_[irow][icol] == cA2DStateNeedInterp )
		nrtobeinterp++;
	    else
	    {
		if ( nrtobeinterp > aie_.maxholesize_ )
		{
		    havebighole = true;
		    state_[irow][icol-1] = cA2DStateMarkedForKeepUdf;
		}

		nrtobeinterp = 0;
	    }
	}
    }

    for ( int icol=0; icol<aie_.nrcols_; icol++ )
    {
	int nrtobeinterp = 0;
	for ( int irow=0; irow<aie_.nrrows_; irow++ )
	{
	    if ( state_[irow][icol] == cA2DStateNeedInterp )
		nrtobeinterp++;
	    else
	    {
		if ( nrtobeinterp > aie_.maxholesize_ )
		{
		    havebighole = true;
		    state_[irow-1][icol] = cA2DStateMarkedForKeepUdf;
		}

		nrtobeinterp = 0;
	    }
	}
    }

    return havebighole;
}


#define mA2DInterpNeedsReplace(r,c) \
    (state_[r][c] == cA2DStateNeedInterp \
     || state_[r][c] == cA2DStateMarkedForKeepUdf)


void Extension2DInterpolExecutor::floodFill4KeepUdf( int seedrow, int seedcol )
{
    for ( int irow = seedrow;
	    seedcol < aie_.nrcols_ && irow < aie_.nrrows_ &&
	    mA2DInterpNeedsReplace(irow,seedcol); irow++ )
	state_[irow][seedcol] = cA2DStateKeepUdf;

    for ( int irow = seedrow - 1;
	    seedcol > -1 && irow > -1 &&
	    mA2DInterpNeedsReplace(irow,seedcol); irow-- )
	state_[irow][seedcol] = cA2DStateKeepUdf;

    if ( seedcol > 0 )
	handleAdjCol( seedrow, seedcol, -1 );

    if ( seedcol < aie_.nrcols_-1 )
	handleAdjCol( seedrow, seedcol, 1 );
}


void Extension2DInterpolExecutor::handleAdjCol( int seedrow, int seedcol,
						int step)
{
    for ( int irow = seedrow;
	    seedcol < aie_.nrcols_ && irow < aie_.nrrows_ &&
	    state_[irow][seedcol] == cA2DStateKeepUdf; irow++ )
    {
	if ( mA2DInterpNeedsReplace( irow, seedcol+step ) )
	    floodFill4KeepUdf( irow, seedcol+step );
    }

    for ( int irow = seedrow - 1;
	    seedcol > -1 && irow > -1 &&
	    state_[irow][seedcol] == cA2DStateKeepUdf; irow-- )
    {
	if ( mA2DInterpNeedsReplace( irow, seedcol+step ) )
	    floodFill4KeepUdf( irow, seedcol+step );
    }
}


int Extension2DInterpolExecutor::nextStep()
{
    if ( !state_ )
    {
	curmsg_ = tr("Memory full");
	return Executor::ErrorOccurred();
    }

    if ( mIsUdf(diagdist_) )
    {
	diagdist_ = Math::Sqrt( aie_.colstep_*aie_.colstep_ +
				aie_.rowstep_ * aie_.rowstep_ );

	adjustInitialStates();
	if ( aie_.maxholesize_ > 0 )
	{
	    if ( markBigHoles() ) //handle big hole
	    {
		for ( int irow=0; irow<aie_.nrrows_; irow++ )
		{
		    for ( int icol=0; icol<aie_.nrcols_; icol++ )
		    {
			if ( state_[irow][icol] == cA2DStateMarkedForKeepUdf )
			    floodFill4KeepUdf( irow, icol );
		    }
		}
	    }
	}

	curmsg_ = uiStrings::sInterpolating();
	return Executor::MoreToDo();
    }

    curlvl_++;
    if ( aie_.getNrSteps() > 0 && aie_.getNrSteps() <= curlvl_ )
	return Executor::Finished();

    bool haveinterpolated = false;
    for ( int irow=0; irow<aie_.nrrows_; irow++ )
    {
	for ( int icol=0; icol<aie_.nrcols_; icol++ )
	{
	    if ( state_[irow][icol] == cA2DStateNeedInterp
		    && doInterpolate(irow,icol) )
		haveinterpolated = true;
	}
    }

    if ( !haveinterpolated )
    {
	curmsg_ = uiStrings::sFinished();
	return Executor::Finished();
    }

    return Executor::MoreToDo();
}
