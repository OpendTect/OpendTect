/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Apr 2010
-*/


#include "seiscube2linedata.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "seisread.h"
#include "seisselection.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisinfo.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "ioobj.h"

class Seis2DFrom3DGeomIDProvider : public GeomIDProvider
{
public:
Seis2DFrom3DGeomIDProvider( const Seis2DFrom3DExtractor& extr )
    : extr_(extr)
{}

Pos::GeomID geomID() const
{ return extr_.curGeomID(); }

const Seis2DFrom3DExtractor& extr_;
};


Seis2DFrom3DExtractor::Seis2DFrom3DExtractor(
			const IOObj& cubein, const IOObj& dsout,
			const TypeSet<Pos::GeomID>& geomids )
    : Executor("Extract 3D data into 2D lines")
    , rdr_(*new SeisTrcReader(&cubein))
    , wrr_(*new SeisTrcWriter(&dsout))
    , geomids_(geomids)
    , nrdone_(0)
    , totalnr_(0)
    , curgeom2d_(0)
    , curlineidx_(-1)
    , curtrcidx_(-1)
{
    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	mDynamicCastGet( const Survey::Geometry2D*, geom2d,
			 Survey::GM().getGeometry(geomids[idx]) );
	if ( geom2d )
	    totalnr_ += geom2d->data().positions().size();
    }
}


Seis2DFrom3DExtractor::~Seis2DFrom3DExtractor()
{
    delete &wrr_; delete &rdr_;
}


Pos::GeomID Seis2DFrom3DExtractor::curGeomID() const
{
    return geomids_.validIdx(curlineidx_) ? geomids_[curlineidx_]
					  : Survey::GM().cUndefGeomID();
}

#define mErrRet(s) { msg_ = s; return ErrorOccurred(); }

int Seis2DFrom3DExtractor::goToNextLine()
{
    curlineidx_++;
    if ( curlineidx_ >= geomids_.size() )
	return Finished();

    if ( !curlineidx_ && !rdr_.prepareWork() )
	mErrRet( rdr_.errMsg() )

    mDynamicCast( const Survey::Geometry2D*, curgeom2d_,
		  Survey::GM().getGeometry(geomids_[curlineidx_]) );
    if ( !curgeom2d_ )
	mErrRet(tr("Line geometry not available"))

    Seis::SelData* newseldata = Seis::SelData::get( Seis::Range );
    newseldata->setGeomID( geomids_[curlineidx_] );
    wrr_.setSelData( newseldata );
    curtrcidx_ = 0;
    const GeomIDProvider* gip = wrr_.geomIDProvider();
    if ( !gip ) gip = new Seis2DFrom3DGeomIDProvider( *this );
    wrr_.setGeomIDProvider( gip );
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
    const PosInfo::Line2DPos& curpos =
		curgeom2d_->data().positions()[curtrcidx_++];
    if ( !rdr_.seisTranslator()->goTo( SI().transform(curpos.coord_) ) )
	return MoreToDo();

    SeisTrc trc;
    if ( !rdr_.get(trc) )
	return MoreToDo();

    trc.info().nr = curpos.nr_;
    trc.info().refnr = sCast(float,curpos.nr_);
    trc.info().coord = curpos.coord_;
    if ( !wrr_.put(trc) )
	mErrRet( wrr_.errMsg() )

    nrdone_++;
    return MoreToDo();
}
