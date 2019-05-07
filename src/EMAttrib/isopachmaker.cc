/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2011
________________________________________________________________________

-*/

#include "isopachmaker.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "executor.h"
#include "emsurfaceauxdata.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "unitofmeasure.h"

static const int sBlockSize = 1000;

const char* IsochronMaker::sKeyHorizonID()
{ return "Horizon ID"; }
const char* IsochronMaker::sKeyCalculateToHorID()
{ return "Calculate to Horizon"; }
const char* IsochronMaker::sKeyAttribName()
{ return "Attribute name"; }
const char* IsochronMaker::sKeyOutputInMilliSecYN()
{ return "Output in milliseconds"; }
const char* IsochronMaker::sKeyIsOverWriteYN()
{ return "Is overwrite"; }

IsochronMaker::IsochronMaker( const EM::Horizon3D& hor1,
			    const EM::Horizon3D& hor2,
			    const char* attrnm, int dataidx, DataPointSet* dps )
    : Executor("Creating Isochron")
    , hor1_(hor1)
    , hor2_(hor2)
    , msg_(tr("Creating %1").arg(uiStrings::sIsoMapType(SI().zIsTime())))
    , dataidx_(dataidx)
    , dps_(dps)
    , inmsec_(false)
    , sidcolidx_(mUdf(int))
{
    iter_ = hor1.createIterator();
    totnr_ = iter_->approximateSize();

    if ( dps_ )
    {
	const DataColDef sidcol( "Section ID" );
	if ( dps_->dataSet().findColDef(sidcol,PosVecDataSet::NameExact)==-1 )
	    dps_->dataSet().add( new DataColDef(sidcol) );

	sidcolidx_ = dps_->dataSet().findColDef(
		sidcol, PosVecDataSet::NameExact ) - dps_->nrFixedCols();
	dps_->dataSet().add( new DataColDef(attrnm) );
    }

    nrdone_ = 0;
}


IsochronMaker::~IsochronMaker()
{
    delete iter_;
}


int IsochronMaker::nextStep()
{
    if ( dataidx_ < 0 )
	return finishWork();

    mAllocVarLenArr( float, vals, dps_ ? dps_->bivSet().nrVals() : 0 );
    int startsourceidx = mUdf(int);
    if ( dps_ )
    {
	for ( int idx=0; idx<dps_->bivSet().nrVals(); idx++ )
	    vals[idx] = mUdf(float);

	const int nrfixedcols = dps_->nrFixedCols();
	vals[sidcolidx_+nrfixedcols] = 0;
	startsourceidx = nrfixedcols + (sidcolidx_ ? 0 : 1);
    }

    for ( int idx=0; idx<sBlockSize; idx++ )
    {
	const EM::PosID posid = iter_->next();
	if ( posid.isInvalid() )
	    return finishWork();

	nrdone_++;
	const Coord3 pos1( hor1_.getPos( posid ) );
	const float z1 = (float) pos1.z_;
	const float z2 = (float) hor2_.getPos( posid ).z_;
	if ( mIsUdf(z1) || mIsUdf(z2) )
	{
	    hor1_.auxdata.setAuxDataVal( dataidx_, posid, mUdf(float) );
	    continue;
	}

	float th = z1 > z2 ? z1 - z2 : z2 - z1;
	if ( inmsec_ )
	    th *= 1000;

	hor1_.auxdata.setAuxDataVal( dataidx_, posid, th );

	if ( dps_ && !mIsUdf(startsourceidx) )
	{
	    const DataPointSet::Pos dpspos( pos1 );
	    vals[0] = z1;
	    vals[startsourceidx] = th;
	    dps_->bivSet().add( dpspos.binID(), vals );
	}
    }

    if ( dps_ )
	dps_->dataChanged();

    return MoreToDo();
}


int IsochronMaker::finishWork()
{
    if ( dps_ )
    {
	dps_->dataChanged();
	if ( dps_->isEmpty() )
	{
	    msg_ = tr("No thickness values collected");
	    return ErrorOccurred();
	}
    }

    if ( dataidx_ >= 0 )
    {
	const UnitOfMeasure* uom = UoMR().get( inmsec_
				    ? UnitOfMeasure::sKeyMilliSeconds()
				    : UnitOfMeasure::sKeySeconds() );
	hor1_.auxdata.setUnit( dataidx_, uom );
    }
    return Finished();
}


bool IsochronMaker::saveAttribute( const EM::Horizon3D* hor, int attribidx,
				  bool overwrite, od_ostream* strm )
{
    PtrMan<Executor> datasaver =
			hor->auxdata.auxDataSaver( attribidx, overwrite );
    if ( !(datasaver && datasaver->go(strm,false,false)) )
	return false;

    return true;
}
