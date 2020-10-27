/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Wayne Mogg
 * DATE     : Oct 2020
-*/


#include "segy2d.h"

#include "dirlist.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "ptrman.h"
#include "segytr.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seisseldata.h"
#include "seisbuf.h"
#include "staticstring.h"
#include "survgeom2d.h"
#include "survgeommgr.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "uistrings.h"

#define mCapChar '^'

int SEGY2DLineIOProvider::factid_
	= (S2DLIOPs() += new SEGY2DLineIOProvider).size() - 1;


SEGY2DLineIOProvider::SEGY2DLineIOProvider()
	: Seis2DLineIOProvider(SEGYSeisTrcTranslator::translKey() )
{
}


class SEGY2DTraceGetter : public Seis2DTraceGetter
{
public:

    SEGY2DTraceGetter( const IOObj& obj, Pos::GeomID geomid,
		       const Seis::SelData* sd )
    : Seis2DTraceGetter(obj,geomid,nullptr)
    {
    }

    void mkTranslator() const
    {
	tr_ = new SEGYSeisTrcTranslator( "SEG-Y", "SEGY" );
	if ( !tr_ )
	{
	    setErrMsgForNoTrMade();
	    return;
	}

	tr_->setIs2D( true );
	tr_->usePar( ioobj_.pars() );
	auto* conn = ioobj_.getConn( true );
	if ( !conn || conn->isBad() )
	{
	    setErrMsgForNoTrMade();
	    deleteAndZeroPtr( tr_ );
	    return;
	}
	if ( !tr_->initRead( conn ) )
	{
	    initmsg_ = uiStrings::phrCannotRead(
					toUiString(ioobj_.mainFileName()) );
		deleteAndZeroPtr( tr_ );
	}
    }
};


bool SEGY2DLineIOProvider::isEmpty( const IOObj& obj,
				    Pos::GeomID geomid ) const
{
    const BufferString fnm = obj.mainFileName();
    return fnm.isEmpty() || File::isEmpty(fnm);
}


uiRetVal SEGY2DLineIOProvider::getGeomIDs( const IOObj& obj,
					   GeomIDSet& geomids ) const
{
    geomids.erase();
    Pos::GeomID geomid;
    const BufferString lnm = obj.pars().find( sKey::LineName() );
    if ( !lnm.isEmpty() )
	geomid = SurvGeom::getGeomID( lnm );
    else
	obj.pars().get( sKey::GeomID(), geomid );
    if ( !mIsUdfGeomID( geomid ) )
	geomids += geomid;

    return uiRetVal::OK();
}


uiRetVal SEGY2DLineIOProvider::getGeometry( const IOObj& obj,
			Pos::GeomID geomid, PosInfo::Line2DData& geom ) const
{
    uiRetVal uirv;
    const BufferString fnm = obj.mainFileName();
    if ( fnm.isEmpty() || !File::exists(fnm) )
    {
	uirv.set( tr("2D seismic line file '%1' does not exist").arg(fnm) );
	return uirv;
    }
    const auto& cgeom2d = SurvGeom::get2D( geomid );
    if ( cgeom2d.isEmpty() )
    {
	PtrMan<SEGY2DTraceGetter> getter = new SEGY2DTraceGetter( obj, geomid,
								  nullptr );
	if ( !getter )
	    return uiRetVal( tr("Failed to create SEGY2DTraceGetter") );
	geom.setEmpty();
	SeisTrc trc;
	auto& geom2d = const_cast<SurvGeom2D&>( cgeom2d );
	while ( true )
	{
	    uirv = getter->getNext( trc );
	    if ( !uirv.isOK() )
		break;
	    const SeisTrcInfo& trcinfo = trc.info();
	    geom2d.add( trcinfo.coord_, trcinfo.trcNr(), trcinfo.refnr_ );
	}
	if ( isFinished( uirv ) )
	{
	    geom2d.data().setZRange( trc.zRange() );
	    geom = geom2d.data();
	    uiString errmsg;
	    if ( !Survey::GMAdmin().save(geom2d,errmsg) )
		return uiRetVal( errmsg );

	    return uiRetVal::OK();
	}
    }
    else
	geom = cgeom2d.data();
    return uirv;
}


Seis2DTraceGetter* SEGY2DLineIOProvider::getTraceGetter( const IOObj& obj,
		Pos::GeomID geomid, const Seis::SelData* sd, uiRetVal& uirv )
{
    const BufferString fnm = obj.mainFileName();
    if ( fnm.isEmpty() || !File::exists(fnm) )
    {
	uirv.set( tr("2D seismic line file '%1' does not exist").arg(fnm) );
	return 0;
    }

    return new SEGY2DTraceGetter( obj, geomid, sd );
}


Seis2DLinePutter* SEGY2DLineIOProvider::getPutter( const IOObj& obj,
					Pos::GeomID geomid, uiRetVal& uirv )
{
    return nullptr;
}


bool SEGY2DLineIOProvider::getTxtInfo( const IOObj& obj,
					     Pos::GeomID geomid,
					     BufferString& uinf,
					     BufferString& stdinf ) const
{
    const BufferString fnm = obj.mainFileName();
    if ( fnm.isEmpty() || !File::exists(fnm) )
	return false;

    PtrMan<SEGYSeisTrcTranslator> trans = new SEGYSeisTrcTranslator( "SEG-Y",
								     "SEGY" );
    if ( !trans )
	return false;
    trans->usePar( obj.pars() );
    const SeisPacketInfo& pinf = trans->packetInfo();
    uinf = pinf.usrinfo;
    stdinf = pinf.stdinfo;
    return true;
}


bool SEGY2DLineIOProvider::getRanges( const IOObj& obj,
				      Pos::GeomID geomid,
				      StepInterval<int>& trcrg,
				      StepInterval<float>& zrg ) const
{
    const BufferString fnm = obj.mainFileName();
    if ( fnm.isEmpty() || !File::exists(fnm) )
	return false;

    const BufferString lnm = obj.pars().find( sKey::LineName() );
    if ( !lnm.isEmpty() )
	return false;

    TrcKeyZSampling tks;
    SeisTrcTranslator::getRanges( obj, tks, lnm );
    trcrg = tks.hsamp_.trcRange();
    zrg = tks.zsamp_;
    return true;
}


bool SEGY2DLineIOProvider::removeImpl( const IOObj& obj,
					     Pos::GeomID geomid ) const
{
    return true;
}


bool SEGY2DLineIOProvider::renameImpl( const IOObj& obj,
					     const char* newnm ) const
{
    return true;
}
