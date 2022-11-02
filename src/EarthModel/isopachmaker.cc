/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "isopachmaker.h"

#include "emhorizon3d.h"
#include "executor.h"
#include "emsurfaceauxdata.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "ptrman.h"

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
    , msg_(tr("Creating Isochron"))
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
    mAllocVarLenArr( float, vals, dps_ ? dps_->bivSet().nrVals() : 0 );
    int startsourceidx = mUdf(int);
    if ( dps_ )
    {
	for ( int idx=0; idx<dps_->bivSet().nrVals(); idx++ )
	    vals[idx] = mUdf(float);

	const int nrfixedcols = dps_->nrFixedCols();
	vals[sidcolidx_+nrfixedcols] = EM::SectionID::def().asInt();
	startsourceidx = nrfixedcols + (sidcolidx_ ? 0 : 1);
    }

    const Geometry::BinIDSurface* hor2geom =
			hor2_.geometry().geometryElement();
    for ( int idx=0; idx<sBlockSize; idx++ )
    {
	const EM::PosID posid = iter_->next();
	nrdone_++;
	if ( !posid.isValid() )
	    return finishWork();

	const EM::SubID subid = posid.subID();
	const Coord3 pos1( hor1_.getPos( subid ) );
	const float z1 = float( pos1.z );
	if ( mIsUdf(z1) )
	{
	    if ( dataidx_ != -1 )
		hor1_.auxdata.setAuxDataVal( dataidx_, posid, mUdf(float) );
	    continue;
	}

	float z2 = float( hor2_.getPos(subid).z );
	if ( mIsUdf(z2) )
	{
	    const BinID bid = BinID::fromInt64( subid );
	    z2 = float( hor2geom->computePosition(
					Coord(bid.inl(),bid.crl()) ).z );
	    if ( mIsUdf(z2) )
	    {
		if ( dataidx_ != -1 )
		    hor1_.auxdata.setAuxDataVal( dataidx_, posid, mUdf(float) );
		continue;
	    }
	}

	float th = z1 > z2 ? z1 - z2 : z2 - z1;
	if ( inmsec_ )
	    th *= 1000;

	if ( dataidx_ != -1 )
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
