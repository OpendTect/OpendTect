/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		July 2017
________________________________________________________________________

-*/

#include "ui2dhorimporter.h"

#include "uiempartserv.h"
#include "uigeninputdlg.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"

#include "binnedvalueset.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "randcolor.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "tabledef.h"
#include "file.h"
#include "emhorizon2d.h"
#include "emhorizonascio.h"
#include "od_helpids.h"


Horizon2DBulkImporter::Horizon2DBulkImporter( const BufferStringSet& lnms,
			ObjectSet<EM::Horizon2D>& hors,
			const BinnedValueSet* valset, UndefTreat udftreat )
    : Executor("2D Horizon Importer")
    , curlinegeom_(0)
    , hors_(hors)
    , bvalset_(valset)
    , prevlineidx_(-1)
    , nrdone_(0)
    , udftreat_(udftreat)
{
    for ( int lineidx=0; lineidx<lnms.size(); lineidx++ )
	geomids_ += SurvGeom::getGeomID( lnms.get(lineidx) );
}


uiString Horizon2DBulkImporter::message() const
{ return tr("Horizon Import"); }

od_int64 Horizon2DBulkImporter::totalNr() const
{ return bvalset_ ? bvalset_->totalSize() : 0; }

uiString Horizon2DBulkImporter::nrDoneText() const
{ return tr("Positions written:"); }

od_int64 Horizon2DBulkImporter::nrDone() const
{ return nrdone_; }

int Horizon2DBulkImporter::nextStep()
{
    if ( !bvalset_ ) return Executor::ErrorOccurred();
    if ( !bvalset_->next(pos_) ) return Executor::Finished();

    BinID bid;
    const int nrvals = bvalset_->nrVals();
    mAllocVarLenArr( float, vals, nrvals )
    for ( int idx=0; idx<nrvals; idx++ )
	vals[idx] = mUdf(float);

    bvalset_->get( pos_, bid, vals );
    if ( bid.inl() < 0 )
	return Executor::ErrorOccurred();

    const Pos::GeomID geomid( bid.inl() );

    if ( bid.inl() != prevlineidx_ )
    {
	prevlineidx_ = bid.inl();
	prevtrcnrs_ = TypeSet<int>( nrvals, -1 );
	prevtrcvals_ = TypeSet<float>( nrvals, mUdf(float) );

	const auto& geom2d = SurvGeom::get2D( geomid );
	if ( geom2d.isEmpty() )
	    return Executor::ErrorOccurred();

	curlinegeom_ = &geom2d;
	for ( int hdx=0; hdx<hors_.size(); hdx++ )
	    hors_[hdx]->geometry().addLine( geomid );
    }

    const int curtrcnr = bid.crl();
    for ( int validx=0; validx<nrvals; validx++ )
    {
	if ( validx>=hors_.size() )
	    break;

	if ( !hors_[validx] )
	    continue;

	const float curval = vals[validx];
	if ( mIsUdf(curval) && udftreat_==Skip )
	    continue;
	hors_[validx]->setZPos( geomid, curtrcnr, curval, false );

	if ( mIsUdf(curval) )
	    continue;

	const int prevtrcnr = prevtrcnrs_[validx];

	if ( udftreat_==Interpolate && prevtrcnr>=0
				    && abs(curtrcnr-prevtrcnr)>1 )
	{
	    interpolateAndSetVals( validx, geomid, curtrcnr, prevtrcnr,
				   curval, prevtrcvals_[validx] );
	}

	prevtrcnrs_[validx] = curtrcnr;
	prevtrcvals_[validx] = curval;
    }

    nrdone_++;
    return Executor::MoreToDo();
}


void Horizon2DBulkImporter::interpolateAndSetVals( int hidx,
					    Pos::GeomID geomid, int curtrcnr,
				   int prevtrcnr, float curval, float prevval )
{
    if ( !curlinegeom_ )
	return;

    const int nrpos = abs( curtrcnr - prevtrcnr ) - 1;
    const bool isrev = curtrcnr < prevtrcnr;
    PosInfo::Line2DPos curpos, prevpos;
    if ( !curlinegeom_->data().getPos(curtrcnr,curpos)
	|| !curlinegeom_->data().getPos(prevtrcnr,prevpos) )
	return;

    const Coord vec = curpos.coord_ - prevpos.coord_;
    for ( int idx=1; idx<=nrpos; idx++ )
    {
	const int trcnr = isrev ? prevtrcnr - idx : prevtrcnr + idx;
	PosInfo::Line2DPos pos;
	if ( !curlinegeom_->data().getPos(trcnr,pos) )
	    continue;

	const Coord newvec = pos.coord_ - prevpos.coord_;
	const float sq = mCast(float,vec.sqAbs());
	const float prod = mCast(float,vec.dot(newvec));
	const float factor = mIsZero(sq,mDefEps) ? 0 : prod / sq;
	const float val = prevval + factor * ( curval - prevval );
	hors_[hidx]->setZPos( geomid, trcnr, val, false );
    }
}
