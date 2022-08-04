  /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : April 2013
-*/


#include "vishordatahandler.h"
#include "vishorizonsectiondef.h"
#include "visdata.h"

#include "vishorizonsection.h"
#include "zaxistransform.h"
#include "trckeyzsampling.h"
#include "binidvalue.h"
#include "binidsurface.h"
#include "datacoldef.h"
#include "posvecdataset.h"
#include "datapointset.h"


namespace visBase
{

HorizonSectionDataHandler::HorizonSectionDataHandler(
    const HorizonSection* hrsection )
    : horsection_( hrsection )
    , zaxistransformvoi_( -2 )
    , zaxistransform_( 0 )
{

}


HorizonSectionDataHandler::~HorizonSectionDataHandler()
{
    removeZTransform();
}


void HorizonSectionDataHandler::updateZAxisVOI()
{
    if( !horsection_ ) return;

    Geometry::BinIDSurface* geometry = horsection_->geometry_;

    if ( !geometry || zaxistransformvoi_==-1 )
	return;

    if ( !zaxistransform_ || !zaxistransform_->needsVolumeOfInterest() )
	return;

    TrcKeyZSampling cs;
    if ( horsection_->userchangedisplayrg_ )
	cs.hsamp_.set( horsection_->displayrrg_, horsection_->displaycrg_ );
    else
	cs.hsamp_.set( geometry->rowRange(), geometry->colRange() );

    TrcKeySamplingIterator iter( cs.hsamp_ );

    bool first = true;
    BinID curpos;
    while ( iter.next(curpos) )
    {
	const float depth = geometry->getKnot(RowCol(curpos),false).z;
	if ( mIsUdf(depth) )
	    continue;

	if ( first )
	{
	    cs.zsamp_.start = cs.zsamp_.stop = depth;
	    first = false;
	}
	else
	    cs.zsamp_.include( depth );
    }

    if ( first ) return;

    if ( zaxistransformvoi_==-2 )
	zaxistransformvoi_ = zaxistransform_->addVolumeOfInterest( cs, false );
    else
	zaxistransform_->setVolumeOfInterest( zaxistransformvoi_, cs, false );

}


void HorizonSectionDataHandler::setZAxisTransform( ZAxisTransform* zt )
{
    if ( zaxistransform_==zt )
	return ;

    removeZTransform();
    if ( !zt )
	return ;

    zaxistransform_ = zt;
    zaxistransform_->ref();

    if ( horsection_->geometry_ )
	updateZAxisVOI();
}


void HorizonSectionDataHandler::removeZTransform()
{
    if ( !zaxistransform_ )
	return;

    if ( zaxistransformvoi_!=-2 )
	zaxistransform_->removeVolumeOfInterest( zaxistransformvoi_ );

    zaxistransformvoi_ = -2;

    zaxistransform_->unRef();
    zaxistransform_ = 0;
}


class DataPointSetFiller : public ParallelTask
{
public:
DataPointSetFiller( BinIDValueSet& bivs, const HorizonSection& section,
	   const ZAxisTransform* zat, double shift, float* vals )
    : bivs_(bivs)
    , section_(section)
    , zat_(zat), shift_(shift), vals_(vals)
{
}


od_int64 nrIterations() const override
{ return section_.geometry_->nrKnots(); }

protected:
bool doWork( od_int64 start, od_int64 stop, int threadidx ) override
{
    mAllocVarLenArr( float, vals, bivs_.nrVals() );
    for ( int idx=0; idx<bivs_.nrVals(); idx++ )
	vals[idx] = vals_[idx];

    const Array2D<float>* depths = section_.geometry_->getArray();
    if ( !depths ) return true;

    const  StepInterval<int> inlrg = section_.geometry_->rowRange();
    const  StepInterval<int> crlrg = section_.geometry_->colRange();

    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	const BinID bid = section_.geometry_->getKnotRowCol(idx);
	const StepInterval<int> displayrrg = section_.displayrrg_;
	const StepInterval<int> displaycrg = section_.displaycrg_;
	if ( section_.userchangedisplayrg_ &&
	    ( !displayrrg.includes(bid.inl(), false) ||
	      !displaycrg.includes(bid.crl(), false) ||
	    ((bid.inl()-displayrrg.start)%displayrrg.step) ||
	    ((bid.crl()-displaycrg.start)%displaycrg.step) ) )
	    continue;

	const int inlidx = inlrg.getIndex( bid.inl() );
	const int crlidx = crlrg.getIndex( bid.crl() );
	float zval = depths->get( inlidx, crlidx );
	if ( mIsUdf(zval) )
	    continue;

	if ( shift_ )
	{
	    if ( !zat_ )
		zval += shift_;
	    else
	    {
		zval = zat_->transform( BinIDValue(bid,zval) );
		if ( mIsUdf(zval) )
		    continue;

		zval += shift_;

		zval = zat_->transformBack( BinIDValue(bid,zval) );
		if ( mIsUdf(zval) )
		    continue;
	    }
	}

	vals[0] = zval;
	BinIDValueSet::SPos bidpos = bivs_.find( bid );
	bivs_.set( bidpos, vals );
    }

    return true;
}

    BinIDValueSet&		bivs_;
    const HorizonSection&	section_;
    const ZAxisTransform*	zat_;
    double			shift_;
    float*			vals_;
};


void HorizonSectionDataHandler::generatePositionData( DataPointSet& dtpntset,
	double zshift, int sectionid ) const
{
    if ( !horsection_ || !horsection_->geometry_ ) return;

    if ( zaxistransform_ && zaxistransformvoi_>=0 )
    {
	if ( !zaxistransform_->loadDataIfMissing(zaxistransformvoi_) )
	    return;
    }

    const char* hrsectionid = "Section ID";
    const DataColDef sidcol( hrsectionid );
    if ( dtpntset.dataSet().findColDef(sidcol,PosVecDataSet::NameExact)==-1 )
	dtpntset.dataSet().add( new DataColDef(sidcol) );

    const int sidcolidx =  dtpntset.dataSet().findColDef(
	sidcol, PosVecDataSet::NameExact ) - dtpntset.nrFixedCols();

    BinIDValueSet& bivs = dtpntset.bivSet();
    mAllocVarLenArr( float, vals, bivs.nrVals() );
    for ( int idx=0; idx<bivs.nrVals(); idx++ )
	vals[idx] = mUdf(float);

    vals[sidcolidx+dtpntset.nrFixedCols()] = sectionid;

    const int nrknots = horsection_->geometry_->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
    {
	const BinID bid = horsection_->geometry_->getKnotRowCol( idx );
	bivs.add( bid );
    }

    DataPointSetFiller filler( bivs, *horsection_, zaxistransform_,
			       zshift, vals );
    filler.execute();
}

} // namespace visBase
