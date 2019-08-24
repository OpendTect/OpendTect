/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Apr 2010
-*/


#include "seiscube2linedata.h"
#include "posinfo2d.h"
#include "seisprovider.h"
#include "seisseldata.h"
#include "seisstorer.h"
#include "seistrc.h"
#include "seisinfo.h"
#include "survgeom2d.h"

Seis2DFrom3DExtractor::Seis2DFrom3DExtractor(
			const IOObj& cubein, const IOObj& dsout,
			const GeomIDSet& geomids )
    : Executor("Extract 3D data into 2D lines")
    , storer_(*new Storer(dsout))
    , geomids_(geomids)
    , prov_(0)
    , nrdone_(0)
    , totalnr_(0)
    , curgeom2d_(0)
    , curlineidx_(-1)
    , curtrcidx_(-1)
{
    prov_ = Provider::create( cubein, &uirv_ );
    if ( !prov_ )
	return;

    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	const auto& geom2d = Geometry2D::get( geomids[idx] );
	totalnr_ += geom2d.data().positions().size();
    }
}


Seis2DFrom3DExtractor::~Seis2DFrom3DExtractor()
{
    delete &storer_; delete prov_;
}


Pos::GeomID Seis2DFrom3DExtractor::curGeomID() const
{
    return geomids_.validIdx(curlineidx_) ? geomids_[curlineidx_] : mUdfGeomID;
}

#define mErrRet(s) { uirv_.add( s ); return ErrorOccurred(); }

int Seis2DFrom3DExtractor::goToNextLine()
{
    if ( !prov_ )
	return ErrorOccurred();

    curlineidx_++;
    if ( curlineidx_ >= geomids_.size() )
	return Finished();

    const auto geomid = geomids_[curlineidx_];
    curgeom2d_ = &SurvGeom::get2D( geomid );
    if ( curgeom2d_->isEmpty() )
	mErrRet(tr("Line geometry not available"))

    curtrcidx_ = 0;
    return MoreToDo();
}


int Seis2DFrom3DExtractor::nextStep()
{
    if ( !curgeom2d_ || curtrcidx_ >= curgeom2d_->data().positions().size() )
	return goToNextLine();

    return handleTrace();
}


int Seis2DFrom3DExtractor::handleTrace()
{
    SeisTrc trc;
    uirv_ = prov_->getNext( trc );
    if ( !uirv_.isOK() )
	return isFinished(uirv_) ? Finished() : ErrorOccurred();

    const PosInfo::Line2DPos& curpos =
		curgeom2d_->data().positions()[curtrcidx_++];
    trc.info().setPos( curgeom2d_->geomID(), curpos.nr_ );
    trc.info().coord_ = curpos.coord_;
    uirv_ = storer_.put( trc );
    if ( !uirv_.isOK() )
	return ErrorOccurred();

    nrdone_++;
    return MoreToDo();
}
