/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : April 2017
-*/


#include "seisblockstr.h"

#include "seisblocksreader.h"
#include "seisblockswriter.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seisselection.h"
#include "survgeom3d.h"
#include "ioman.h"
#include "posinfo.h"


BlocksSeisTrcTranslator::BlocksSeisTrcTranslator( const char* s1,
							  const char* s2 )
    : SeisTrcTranslator(s1,s2)
    , rdr_(0)
    , wrr_(0)
{
}


BlocksSeisTrcTranslator::~BlocksSeisTrcTranslator()
{
    cleanUp();
}


bool BlocksSeisTrcTranslator::close()
{
    // stop (potentially early)
    delete rdr_; rdr_ = 0;
    delete wrr_; wrr_ = 0;
    return SeisTrcTranslator::close();
}


void BlocksSeisTrcTranslator::cleanUp()
{
    // prepare for re-initialization
    SeisTrcTranslator::cleanUp();
}


bool BlocksSeisTrcTranslator::initRead_()
{
    StreamConn& sconn = *static_cast<StreamConn*>( conn_ );
    rdr_ = new Seis::Blocks::Reader( sconn.iStream() );
    if ( !rdr_->state().isOK() )
    {
	errmsg_ = rdr_->state();
	return false;
    }
    else if ( !rdr_->hGeom().isCompatibleWith( Survey::Geometry::default3D() ) )
    {
	errmsg_ = tr("The cube is not compatible with the survey setup");
	return false;
    }

    pinfo_.usrinfo = rdr_->cubeName();
    pinfo_.stdinfo = rdr_->infoFileName();
    pinfo_.fullyrectandreg = false;
    pinfo_.cubedata = &rdr_->positions();
    rdr_->positions().getRanges( pinfo_.inlrg, pinfo_.crlrg );
    insd_.start = rdr_->zGeom().start;
    insd_.step = rdr_->zGeom().step;
    innrsamples_ = rdr_->zGeom().nrSteps() + 1;

    const DataCharacteristics dc( rdr_->fPRep() );
    const int nrcomps = rdr_->nrComponents();
    for ( int icomp=0; icomp<nrcomps; icomp++ )
	addComp( dc, rdr_->componentNames().get(icomp) );

    return true;
}


bool BlocksSeisTrcTranslator::initWrite_( const SeisTrc& trc )
{
    StreamConn& sconn = *static_cast<StreamConn*>( conn_ );
    const BufferString infofnm( sconn.fileName() );
    const DBKey dbky = sconn.linkedTo();
    sconn.close( true ); // this preserves the original file for now

    wrr_ = new Seis::Blocks::Writer;
    wrr_->setFullPath( infofnm );

    PtrMan<IOObj> ioobj = DBM().get( dbky );
    if ( ioobj )
    {
	OD::FPDataRepType fprep;
	if ( DataCharacteristics::getUserTypeFromPar(ioobj->pars(),fprep) )
	    wrr_->setFPRep( fprep );
	ZDomain::Def zdom = ZDomain::Def::get( ioobj->pars() );
	wrr_->setZDomain( zdom );
    }

    for ( int idx=0; idx<trc.nrComponents(); idx++ )
    {
	DataCharacteristics dc( trc.data().getInterpreter(idx)->dataChar() );
	addComp( dc, 0 );
    }
    insd_ = trc.info().sampling_;
    innrsamples_ = trc.size();

    return true;
}


bool BlocksSeisTrcTranslator::commitSelections_()
{
    if ( seldata_ && rdr_ )
	rdr_->setSelData( seldata_ );
    if ( wrr_ && compnms_ && !compnms_->isEmpty() )
    {
	for ( int icomp=0; icomp<compnms_->size(); icomp++ )
	    wrr_->addComponentName( compnms_->get(icomp) );
    }

    return true;
}


bool BlocksSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    uiRetVal uirv = rdr_->getTrcInfo( ti );
    if ( uirv.isError() )
    {
	if ( isFinished(uirv) )
	    errmsg_.setEmpty();
	else
	    errmsg_ = uirv;
	return false;
    }
    return true;
}


bool BlocksSeisTrcTranslator::read( SeisTrc& trc )
{
    uiRetVal uirv = rdr_->getNext( trc );
    if ( uirv.isError() )
    {
	if ( isFinished(uirv) )
	    errmsg_.setEmpty();
	else
	    errmsg_ = uirv;
	return false;
    }
    return true;
}


bool BlocksSeisTrcTranslator::skip( int ntrcs )
{
    uiRetVal uirv = rdr_->skip( ntrcs );
    if ( uirv.isError() )
	errmsg_ = uirv;
    return uirv.isOK();
}


bool BlocksSeisTrcTranslator::goTo( const BinID& bid )
{
    return rdr_->goTo( bid );
}


bool BlocksSeisTrcTranslator::write( const SeisTrc& trc )
{
    uiRetVal uirv = wrr_->add( trc );
    if ( uirv.isError() )
	errmsg_ = uirv;
    return uirv.isOK();
}


bool BlocksSeisTrcTranslator::getGeometryInfo( PosInfo::CubeData& cd ) const
{
    cd = rdr_->positions();
    return true;
}


int BlocksSeisTrcTranslator::estimatedNrTraces() const
{
    return rdr_->positions().totalSize();
}


void BlocksSeisTrcTranslator::usePar( const IOPar& iop )
{
    SeisTrcTranslator::usePar( iop );
    // DataCharacteristics::getUserTypeFromPar( iopar, preseldatatype_ );
}
