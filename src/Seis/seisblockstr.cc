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


SeisBlocksSeisTrcTranslator::SeisBlocksSeisTrcTranslator( const char* s1,
							  const char* s2 )
    : SeisTrcTranslator(s1,s2)
    , rdr_(0)
    , wrr_(0)
{
}


SeisBlocksSeisTrcTranslator::~SeisBlocksSeisTrcTranslator()
{
    cleanUp();
}


bool SeisBlocksSeisTrcTranslator::close()
{
    // stop (potentially early)
    delete rdr_; rdr_ = 0;
    delete wrr_; wrr_ = 0;
    return SeisTrcTranslator::close();
}


void SeisBlocksSeisTrcTranslator::cleanUp()
{
    // prepare for re-initialization
    SeisTrcTranslator::cleanUp();
}


bool SeisBlocksSeisTrcTranslator::initRead_()
{
    StreamConn& sconn = *static_cast<StreamConn*>( conn_ );
    rdr_ = new Seis::Blocks::Reader( sconn.iStream() );
    sconn.close();
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
    pinfo_.stdinfo = rdr_->mainFileName();
    pinfo_.fullyrectandreg = false;
    pinfo_.cubedata = &rdr_->positions();
    pinfo_.inlrg = rdr_->inlRange();
    pinfo_.crlrg = rdr_->crlRange();
    insd_.start = rdr_->zGeom().start;
    insd_.step = rdr_->zGeom().step;
    innrsamples_ = rdr_->zGeom().nrSteps() + 1;

    const DataCharacteristics dc( rdr_->fPRep() );
    const int nrcomps = rdr_->nrComponents();
    for ( int icomp=0; icomp<nrcomps; icomp++ )
	addComp( dc, rdr_->componentNames().get(icomp) );

    return true;
}


bool SeisBlocksSeisTrcTranslator::initWrite_( const SeisTrc& trc )
{
    StreamConn& sconn = *static_cast<StreamConn*>( conn_ );
    const BufferString mainfnm( sconn.fileName() );
    const DBKey dbky = sconn.linkedTo();
    sconn.close( true ); // this preserves the original file for now

    wrr_ = new Seis::Blocks::Writer;
    File::Path fp( mainfnm );
    fp.setExtension( 0 );
    wrr_->setFileNameBase( fp.fileName() );
    fp.setFileName( 0 );
    wrr_->setBasePath( fp );

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


bool SeisBlocksSeisTrcTranslator::commitSelections_()
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


bool SeisBlocksSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
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


bool SeisBlocksSeisTrcTranslator::read( SeisTrc& trc )
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


bool SeisBlocksSeisTrcTranslator::skip( int ntrcs )
{
    uiRetVal uirv = rdr_->skip( ntrcs );
    if ( uirv.isError() )
	errmsg_ = uirv;
    return uirv.isOK();
}


bool SeisBlocksSeisTrcTranslator::goTo( const BinID& bid )
{
    return rdr_->goTo( bid );
}


bool SeisBlocksSeisTrcTranslator::write( const SeisTrc& trc )
{
    uiRetVal uirv = wrr_->add( trc );
    if ( uirv.isError() )
	errmsg_ = uirv;
    return uirv.isOK();
}


bool SeisBlocksSeisTrcTranslator::getGeometryInfo( PosInfo::CubeData& cd ) const
{
    cd = rdr_->positions();
    return true;
}


int SeisBlocksSeisTrcTranslator::estimatedNrTraces() const
{
    return rdr_->positions().totalSize();
}


void SeisBlocksSeisTrcTranslator::usePar( const IOPar& iop )
{
    SeisTrcTranslator::usePar( iop );
    // DataCharacteristics::getUserTypeFromPar( iopar, preseldatatype_ );
}
