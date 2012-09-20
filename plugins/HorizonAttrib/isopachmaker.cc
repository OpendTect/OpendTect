/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2011
 RCS:           $Id$
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

static const int sBlockSize = 1000;

const char* IsopachMaker::sKeyHorizonID()
{ return "Horizon ID"; }
const char* IsopachMaker::sKeyCalculateToHorID()
{ return "Calculate to Horizon"; }
const char* IsopachMaker::sKeyAttribName()
{ return "Attribute name"; }
const char* IsopachMaker::sKeyOutputInMilliSecYN()
{ return "Output in milliseconds"; }
const char* IsopachMaker::sKeyIsOverWriteYN()
{ return "Is overwrite"; }

IsopachMaker::IsopachMaker( const EM::Horizon3D& hor1,
			    const EM::Horizon3D& hor2,
			    const char* attrnm, int dataidx, DataPointSet* dps )
    : Executor("Create isopach")
    , hor1_(hor1)
    , hor2_(hor2)
    , msg_("Creating isopach")
    , dataidx_(dataidx)
    , dps_(dps)
    , sectid1_(hor1.sectionID(0))
    , sectid2_(hor2.sectionID(0))
    , inmsec_(false)
    , sidcolidx_(mUdf(int))
{
    iter_ = hor1.createIterator( sectid1_ );
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


IsopachMaker::~IsopachMaker()
{
    delete iter_;
}


int IsopachMaker::nextStep()
{
    mAllocVarLenArr( float, vals, dps_ ? dps_->bivSet().nrVals() : 0 );
    int startsourceidx = mUdf(int);
    if ( dps_ )
    {
	for ( int idx=0; idx<dps_->bivSet().nrVals(); idx++ )
	    vals[idx] = mUdf(float);

	const int nrfixedcols = dps_->nrFixedCols();
	vals[sidcolidx_+nrfixedcols] = sectid1_;
	startsourceidx = nrfixedcols + (sidcolidx_ ? 0 : 1);
    }

    for ( int idx=0; idx<sBlockSize; idx++ )
    {
	const EM::PosID posid = iter_->next();
	nrdone_++;
	if ( posid.objectID() < 0 )
	    return finishWork();

	if ( posid.sectionID() != sectid1_ )
	    continue;

	const EM::SubID subid = posid.subID();
	const Coord3 pos1( hor1_.getPos( sectid1_, subid ) );
	const float z1 = (float) pos1.z;
	const float z2 = (float) hor2_.getPos( sectid2_, subid ).z;
	if ( mIsUdf(z1) || mIsUdf(z2) )
	{
	    if ( dataidx_ != -1 )
		hor1_.auxdata.setAuxDataVal( dataidx_, posid, mUdf(float) );
	    continue;
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


int IsopachMaker::finishWork()
{
    if ( dps_ )
    {
	dps_->dataChanged();
	if ( dps_->isEmpty() )
	{
	    msg_ = "No thickness values collected";
	    return ErrorOccurred();
	}
    }

    return Finished();
}


bool IsopachMaker::saveAttribute( const EM::Horizon3D* hor, int attribidx,
				  bool overwrite, std::ostream* strm )
{
    PtrMan<Executor> datasaver =
			hor->auxdata.auxDataSaver( attribidx, overwrite );
    if ( !(datasaver && datasaver->execute(strm,false,false,0)) )
	return false;

    return true;
}
