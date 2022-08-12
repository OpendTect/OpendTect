/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2007
________________________________________________________________________

-*/

#include "emhor2dto3d.h"

#include "array2dinterpolimpl.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emrowcoliterator.h"
#include "arrayndimpl.h"
#include "array2dinterpol.h"
#include "trckeyzsampling.h"
#include "survinfo.h"
#include "uistrings.h"

namespace EM
{

class Hor2DTo3DSectionData
{
public:
Hor2DTo3DSectionData( const BinID& minbid, const BinID& maxbid,
		      const BinID& step )
    : arr_( getSz(minbid.inl(),maxbid.inl(),step.inl()),
	    getSz(minbid.crl(),maxbid.crl(),step.crl()) )
    , count_( getSz(minbid.inl(),maxbid.inl(),step.inl()),
	      getSz(minbid.crl(),maxbid.crl(),step.crl()) )
{
    inlsz_ = count_.info().getSize( 0 );
    crlsz_ = count_.info().getSize( 1 );

    count_.setAll( 0 );
    arr_.setAll( mUdf(float) );

    hs_.start_ = minbid;
    hs_.step_ = step;
    hs_.stop_.inl() = hs_.start_.inl() + hs_.step_.inl() * (inlsz_ - 1);
    hs_.stop_.crl() = hs_.start_.crl() + hs_.step_.crl() * (crlsz_ - 1);
}


int getSz( int start, int stop, int step ) const
{
    int diff = stop - start;
    int ret = diff / step + 1;
    if ( diff % step ) ret += 1;
    return ret;
}


void add( const BinID& bid, float z )
{
    float inldist = (bid.inl() - hs_.start_.inl()) / ((float)hs_.step_.inl());
    float crldist = (bid.crl() - hs_.start_.crl()) / ((float)hs_.step_.crl());
    const int inlidx = mNINT32(inldist); const int crlidx = mNINT32(crldist);
    if ( inlidx < 0 || inlidx >= inlsz_ || crlidx < 0 || crlidx >= crlsz_ )
	return;

    float curz = arr_.get( inlidx, crlidx );
    if ( mIsUdf(curz) )
	arr_.set( inlidx, crlidx, z );
    else
    {
	int curcount = count_.get( inlidx, crlidx );
	z = (curcount*curz + z) / (curcount + 1);
	arr_.set( inlidx, crlidx, z );
	count_.set( inlidx, crlidx, curcount+1 );
    }
}

    Array2DImpl<int>	count_;
    Array2DImpl<float>	arr_;
    TrcKeySampling	hs_;
    EM::SectionID	sid_;
    int			inlsz_;
    int			crlsz_;
};


Hor2DTo3D::Hor2DTo3D( const Horizon2D& h2d, Array2DInterpol* interp,
		     Horizon3D& h3d, TaskRunner* taskrunner )
    : Executor( "Deriving 3D horizon from 2D" )
    , hor2d_(h2d)
    , hor3d_(h3d)
    , cursectnr_(0)
    , curinterp_( interp )
{
    const TrcKeySampling hrg = SI().sampling(true).hsamp_;
    addSections( hrg );
    fillSections();

    if ( sd_.isEmpty() )
	msg_ = tr("No data in selected area");
    else if ( curinterp_ )
    {
	curinterp_->setOrigin( hrg.start_ );
	const float inldist = hrg.step_.inl()*SI().inlDistance();
	const float crldist = hrg.step_.crl()*SI().crlDistance();

	curinterp_->setRowStep( inldist );
	curinterp_->setColStep( crldist );

	curinterp_->setMaxHoleSize( mUdf(float) );
	const bool issingleline = hor2d_.geometry().nrLines()<2;
	curinterp_->setFillType( issingleline ? Array2DInterpol::Full
					      : Array2DInterpol::ConvexHull );
	const bool res =
		curinterp_->setArray( sd_[cursectnr_]->arr_, taskrunner );
	msg_ = curinterp_->uiMessage();
	if ( !res )
	{
	    deleteAndZeroPtr( curinterp_ );
	    deepErase( sd_ );
	}
    }

    hor3d_.removeAll();
}


void Hor2DTo3D::addSections( const TrcKeySampling& hs )
{
    BinID minbid( mUdf(int), 0 ), maxbid;
    for ( EM::RowColIterator iter(hor2d_); ; )
    {
	const EM::PosID posid = iter.next();
	if ( !posid.isValid() )
	    break;

	const Coord coord = hor2d_.getPos( posid );
	const BinID bid = SI().transform( coord );
	if ( mIsUdf(minbid.inl()) )
	    minbid = maxbid = bid;
	else
	{
	    if ( minbid.inl() > bid.inl() ) minbid.inl() = bid.inl();
	    if ( minbid.crl() > bid.crl() ) minbid.crl() = bid.crl();
	    if ( maxbid.inl() < bid.inl() ) maxbid.inl() = bid.inl();
	    if ( maxbid.crl() < bid.crl() ) maxbid.crl() = bid.crl();
	}
    }

    if ( mIsUdf(minbid.inl()) || minbid == maxbid )
	return;

    if ( curinterp_ &&
	 (minbid.inl()==maxbid.inl() || minbid.crl()==maxbid.crl()) )
    {
	int extendedsize = 1;
	mDynamicCastGet(InverseDistanceArray2DInterpol*, inv, curinterp_ );
	if ( inv && !mIsUdf(inv->getNrSteps()) )
	    extendedsize = inv->getNrSteps();

	minbid.inl() -= extendedsize;
	if ( minbid.inl()<0 ) minbid.inl() = 0;
	minbid.crl() -= extendedsize;
	if ( minbid.crl()<0 ) minbid.crl() = 0;
	maxbid.inl() += extendedsize;
	maxbid.crl() += extendedsize;
    }

    sd_ += new Hor2DTo3DSectionData( minbid, maxbid, hs.step_ );
}


void Hor2DTo3D::fillSections()
{
    for ( int isd=0; isd<sd_.size(); isd++ )
    {
	Coord lastpos = Coord::udf();
	Hor2DTo3DSectionData& sd = *sd_[isd];
	for ( EM::RowColIterator iter(hor2d_); ; )
	{
	    const EM::PosID posid = iter.next();
	    if ( !posid.isValid() )
		break;

	    const Coord3 coord = hor2d_.getPos( posid );
	    const BinID bid = SI().transform( coord );
	    sd.add( bid, (float) coord.z );
	    lastpos = coord;
	}
    }
}


Hor2DTo3D::~Hor2DTo3D()
{
    delete curinterp_;
    deepErase( sd_ );
}


uiString Hor2DTo3D::uiNrDoneText() const
{
    return curinterp_ ? curinterp_->uiNrDoneText() : uiString::emptyString();
}


od_int64 Hor2DTo3D::nrDone() const
{
    return curinterp_ ? curinterp_->nrDone() : -1;
}


od_int64 Hor2DTo3D::totalNr() const
{
    return curinterp_ ? curinterp_->totalNr() : sd_.size();
}


int Hor2DTo3D::nextStep()
{
    if ( !curinterp_ && sd_.isEmpty() )
    {
	msg_ = tr("No data to grid.");
	return Executor::ErrorOccurred();
    }
    else if ( sd_.isEmpty() )
    {
	msg_ = tr( "No data in selected area.");
	return Executor::ErrorOccurred();
    }

    if ( curinterp_ )
    {
	curinterp_->execute();
    }

    const Hor2DTo3DSectionData& sd = *sd_[cursectnr_];

    const bool geowaschecked = hor3d_.enableGeometryChecks( false );

    hor3d_.geometry().geometryElement()->
				expandWithUdf( sd.hs_.start_, sd.hs_.stop_ );

    for ( int inlidx=0; inlidx<sd.inlsz_; inlidx++ )
    {
	for ( int crlidx=0; crlidx<sd.crlsz_; crlidx++ )
	{
	    const BinID bid( sd.hs_.inlRange().atIndex(inlidx),
			     sd.hs_.crlRange().atIndex(crlidx) );
	    const Coord3 pos( SI().transform(bid), sd.arr_.get(inlidx,crlidx) );
	    if ( pos.isDefined() )
		hor3d_.setPos( bid.toInt64(), pos, false );
	}
    }

    hor3d_.geometry().geometryElement()->trimUndefParts();
    hor3d_.enableGeometryChecks( geowaschecked );

    cursectnr_++;
    if ( cursectnr_ >= sd_.size() )
	return Executor::Finished();

    if ( curinterp_ )
    {
	curinterp_->setArray( sd_[cursectnr_]->arr_ );
    }

    return Executor::MoreToDo();
}

} // namespace EM
