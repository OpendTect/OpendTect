/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : April 2017
-*/


#include "seisblockstr.h"

#include "cubedata.h"
#include "file.h"
#include "seisblocksreader.h"
#include "seisblockswriter.h"
#include "seispacketinfo.h"
#include "seisseldata.h"
#include "seistrc.h"
#include "survgeom3d.h"


BlocksSeisTrcTranslator::BlocksSeisTrcTranslator( const char* s1,
						  const char* s2 )
    : SeisTrcTranslator(s1,s2)
    , rdr_(0)
    , wrr_(0)
    , preseldatarep_(OD::AutoDataRep)
{
}


BlocksSeisTrcTranslator::~BlocksSeisTrcTranslator()
{
    cleanUp();
    close();
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
    preseldatarep_ = OD::AutoDataRep;
}


const char* BlocksSeisTrcTranslator::defExtension() const
{
    return Seis::Blocks::Access::sDataFileExt(
		Seis::Blocks::Access::hdf5Active() );
}


void BlocksSeisTrcTranslator::convToConnExpr( BufferString& fnm ) const
{
    if ( !fnm.isEmpty() )
    {
	File::Path fp( fnm );
	fp.setExtension( sInfoFileExtension() );
	fnm = fp.fullPath();
    }
}


bool BlocksSeisTrcTranslator::initRead_()
{
    StreamConn& sconn = *static_cast<StreamConn*>( conn_ );
    const BufferString fnm( sconn.fileName() );
    sconn.close();
    rdr_ = new Seis::Blocks::Reader( fnm );
    if ( !rdr_->state().isOK() )
	{ errmsg_ = rdr_->state(); return false; }

    pinfo_.usrinfo = rdr_->cubeName();
    pinfo_.stdinfo = rdr_->infoFileName();
    pinfo_.cubedata = &rdr_->positions();
    pinfo_.fullyrectandreg = pinfo_.cubedata->isFullyRegular();
    rdr_->positions().getRanges( pinfo_.inlrg, pinfo_.crlrg );
    pinfo_.inlrg.step = rdr_->hGeom().inlRange().step;
    pinfo_.crlrg.step = rdr_->hGeom().crlRange().step;
    insd_.start = rdr_->zGeom().start;
    insd_.step = rdr_->zGeom().step;
    innrsamples_ = rdr_->zGeom().nrSteps() + 1;

    const DataCharacteristics dc( rdr_->dataRep() );
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

    PtrMan<IOObj> ioobj = dbky.getIOObj();
    if ( ioobj )
    {
	OD::DataRepType datarep = OD::AutoDataRep;
	if ( preseldatarep_ != OD::AutoDataRep )
	    datarep = preseldatarep_;
	else
	    DataCharacteristics::getUserTypeFromPar( ioobj->pars(), datarep );
	wrr_->setDataRep( datarep );
	ZDomain::Def zdom = ZDomain::Def::get( ioobj->pars() );
	wrr_->setZDomain( zdom );
	wrr_->setCubeName( ioobj->name() );
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
    if ( rdr_ )
    {
	if ( seldata_ )
	    rdr_->setSelData( seldata_ );
	for ( int idx=0; idx<tarcds_.size(); idx++ )
	    rdr_->compSelected()[idx] = tarcds_[idx]->selected_;
    }
    else if ( wrr_ )
    {
	if ( compnms_ && !compnms_->isEmpty() )
	{
	    for ( int icomp=0; icomp<compnms_->size(); icomp++ )
		wrr_->addComponentName( compnms_->get(icomp) );
	}
	else
	{
	    for ( int idx=0; idx<tarcds_.size(); idx++ )
		wrr_->addComponentName( tarcds_[idx]->name() );
	}
    }
    else
	return false;

    return true;
}


bool BlocksSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    if ( !ensureSelectionsCommitted() )
	return false;

    uiRetVal uirv = rdr_->getTrcInfo( ti );
    if ( uirv.isError() )
    {
	if ( isFinished(uirv) )
	    errmsg_.setEmpty();
	else
	    errmsg_ = uirv;
	return false;
    }
    headerdone_ = true;
    return true;
}


bool BlocksSeisTrcTranslator::readData( TraceData* extbuf )
{
    if ( !ensureSelectionsCommitted() )
	return false;

    TraceData& tdata = extbuf ? *extbuf : *trcdata_;
    uiRetVal uirv = rdr_->getTrcData( tdata );
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


bool BlocksSeisTrcTranslator::writeTrc_( const SeisTrc& trc )
{
    uiRetVal uirv = wrr_->add( trc );
    if ( uirv.isError() )
	errmsg_ = uirv;
    return uirv.isOK();
}


bool BlocksSeisTrcTranslator::getGeometryInfo( LineCollData& cd ) const
{
    cd = rdr_->positions();
    return true;
}


int BlocksSeisTrcTranslator::estimatedNrTraces() const
{
    return rdr_->positions().totalSize();
}


BufferStringSet BlocksSeisTrcTranslator::auxExtensions() const
{
    BufferStringSet extnms = stdAuxExtensions();
    extnms.add( sInfoFileExtension() );
    extnms.add( Seis::Blocks::Access::sKeyOvvwFileExt() );
    return extnms;
}


void BlocksSeisTrcTranslator::usePar( const IOPar& iop )
{
    SeisTrcTranslator::usePar( iop );
    DataCharacteristics::getUserTypeFromPar( iop, preseldatarep_ );
}
