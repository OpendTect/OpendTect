/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisblockstr.h"

#include "seisblocksreader.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seisselection.h"
#include "survgeom3d.h"
#include "ioman.h"
#include "strmprov.h"
#include "posinfo.h"
#include "file.h"
#include "uistrings.h"


BlocksSeisTrcTranslator::BlocksSeisTrcTranslator( const char* s1,
						  const char* s2 )
    : SeisTrcTranslator(s1,s2)
    , rdr_(nullptr)
    , preselfprep_(DataCharacteristics::Auto)
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
    return SeisTrcTranslator::close();
}


void BlocksSeisTrcTranslator::cleanUp()
{
    // prepare for re-initialization
    SeisTrcTranslator::cleanUp();
    preselfprep_ = DataCharacteristics::Auto;
}


void BlocksSeisTrcTranslator::convToConnExpr( BufferString& fnm ) const
{
    if ( !fnm.isEmpty() )
    {
	FilePath fp( fnm );
	fp.setExtension( Seis::sInfoFileExtension() );
	fnm = fp.fullPath();
    }
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
    else if ( read_mode == Seis::Prod
	  && !rdr_->hGeom().isCompatibleWith( Survey::Geometry::default3D() ) )
    {
	errmsg_ = tr("The cube is not compatible with the survey setup");
	return false;
    }

    pinfo_.usrinfo = rdr_->cubeName();
    pinfo_.stdinfo = rdr_->infoFileName();
    pinfo_.cubedata = &rdr_->positions();
    pinfo_.fullyrectandreg = pinfo_.cubedata->isFullyRectAndReg();
    rdr_->positions().getRanges( pinfo_.inlrg, pinfo_.crlrg );
    pinfo_.inlrg.step = rdr_->hGeom().sampling().hsamp_.step_.inl();
    pinfo_.crlrg.step = rdr_->hGeom().sampling().hsamp_.step_.crl();
    insd_.start = rdr_->zGeom().start;
    insd_.step = rdr_->zGeom().step;
    innrsamples_ = rdr_->zGeom().nrSteps() + 1;

    const DataCharacteristics dc( rdr_->fPRep() );
    const int nrcomps = rdr_->nrComponents();
    for ( int icomp=0; icomp<nrcomps; icomp++ )
	addComp( dc, rdr_->componentNames().get(icomp) );

    return true;
}


#define mRetNotImplIn6() \
    { errmsg_ = uiStrings::phrNotImplInThisVersion( "7.X" ); return false; }

bool BlocksSeisTrcTranslator::initWrite_( const SeisTrc& trc )
{
    mRetNotImplIn6();
}


bool BlocksSeisTrcTranslator::commitSelections_()
{
    if ( rdr_ )
    {
	if ( seldata_ )
	    rdr_->setSelData( seldata_ );
	for ( int idx=0; idx<tarcds_.size(); idx++ )
	    rdr_->compSelected()[idx] = tarcds_[idx]->destidx >= 0;
    }
    else
	return false;

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


bool BlocksSeisTrcTranslator::writeTrc_( const SeisTrc& trc )
{
    mRetNotImplIn6();
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


BufferStringSet BlocksSeisTrcTranslator::auxExtensions() const
{
    BufferStringSet extnms;
    extnms.add( "par" );
    extnms.add( "stats" );
    extnms.add( "proc" );
    extnms.add( "info" );
    extnms.add( "ovvw" );
    return extnms;
}


void BlocksSeisTrcTranslator::usePar( const IOPar& iop )
{
    SeisTrcTranslator::usePar( iop );
    DataCharacteristics::getUserTypeFromPar( iop, preselfprep_ );
}
