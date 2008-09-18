/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

static const char* rcsID = "$Id: segyfiledef.cc,v 1.1 2008-09-18 14:55:52 cvsbert Exp $";

#include "segyfiledef.h"
#include "segytr.h"
#include "iopar.h"
#include "keystrs.h"
#include "separstr.h"
#include "survinfo.h"
#include "datapointset.h"

const char* SEGY::FileSpec::sKeyFileNrs = "File numbers";


const char* SEGY::FileSpec::getFileName( int nr ) const
{
    if ( !isMultiFile() )
	return fname_.buf();

    BufferString numbstr( nr );
    BufferString replstr;
    if ( zeropad_ < 2 )
	replstr = numbstr;
    else
    {
	const int numblen = numbstr.size();
	while ( numblen + replstr.size() < zeropad_ )
	    replstr += "0";
	replstr += numbstr;
    }

    static FileNameString ret( fname_ );
    replaceString( ret.buf(), "*", replstr.buf() );
    return ret.buf();
}


void SEGY::FileSpec::fillPar( IOPar& iop ) const
{
    iop.set( sKey::FileName, fname_ );
    if ( mIsUdf(nrs_.start) )
	iop.removeWithKey( sKeyFileNrs );
    else
    {
	FileMultiString fms;
	fms += nrs_.start; fms += nrs_.stop; fms += nrs_.step;
	if ( zeropad_ )
	    fms += zeropad_;
	iop.set( sKeyFileNrs, fms );
    }
}


void SEGY::FileSpec::usePar( const IOPar& iop )
{
    iop.get( sKey::FileName, fname_ );
    getMultiFromString( iop.find(sKeyFileNrs) );
}


void SEGY::FileSpec::getMultiFromString( const char* str )
{
    FileMultiString fms( str );
    const int len = fms.size();
    nrs_.start = len > 0 ? atoi( fms[0] ) : mUdf(int);
    if ( len > 1 )
	nrs_.stop = atoi( fms[1] );
    if ( len > 2 )
	nrs_.step = atoi( fms[2] );
    if ( len > 3 )
	zeropad_ = atoi( fms[3] );
}


const char** SEGY::FilePars::getFmts( bool fr )
{
    return SEGYSeisTrcTranslator::getFmts( fr );
}


void SEGY::FilePars::fillPar( IOPar& iop ) const
{
    iop.set( SEGYSeisTrcTranslator::sExternalNrSamples, ns_ );
    iop.set( SEGYSeisTrcTranslator::sNumberFormat, nameOfFmt(fmt_,forread_) );
    iop.setYN( SegylikeSeisTrcTranslator::sKeyBytesSwapped, byteswapped_ );
}


void SEGY::FilePars::usePar( const IOPar& iop )
{
    iop.get( SEGYSeisTrcTranslator::sExternalNrSamples, ns_ );
    iop.getYN( SegylikeSeisTrcTranslator::sKeyBytesSwapped, byteswapped_ );
    fmt_ = fmtOf( iop.find(SEGYSeisTrcTranslator::sNumberFormat), forread_ );
}


const char* SEGY::FilePars::nameOfFmt( int fmt, bool forread )
{
    const char** fmts = getFmts(true);
    if ( fmt > 0 && fmt < 4 )
	return fmts[fmt];
    if ( fmt == 5 )
	return fmts[4];
    if ( fmt == 8 )
	return fmts[5];

    return forread ? fmts[0] : nameOfFmt( 1, false );
}


int SEGY::FilePars::fmtOf( const char* str, bool forread )
{
    if ( !str || !*str || !isdigit(*str) )
	return forread ? 0 : 1;

    return (int)(*str - '0');
}


SEGY::FileData::FileData( const char* fnm, Seis::GeomType gt )
    : fname_(fnm)
    , geom_(gt)
    , data_(*new DataPointSet(Seis::is2D(gt),false))
    , trcsz_(-1)
    , sampling_(SI().zRange(false).start,SI().zRange(false).step)
    , segyfmt_(0)
    , isrev1_(true)
    , nrstanzas_(0)
{
}


SEGY::FileData::FileData( const SEGY::FileData& fd )
    : fname_(fd.fname_)
    , geom_(fd.geom_)
    , data_(*new DataPointSet(fd.data_))
    , trcsz_(fd.trcsz_)
    , sampling_(fd.sampling_)
    , segyfmt_(fd.segyfmt_)
    , isrev1_(fd.isrev1_)
    , nrstanzas_(fd.nrstanzas_)
{
}


SEGY::FileData::~FileData()
{
    delete &data_;
}


int SEGY::FileData::nrTraces() const
{
    return data_.size();
}


BinID SEGY::FileData::binID( int nr ) const
{
    return data_.binID( nr );
}


Coord SEGY::FileData::coord( int nr ) const
{
    return data_.coord( nr );
}


float SEGY::FileData::offset( int nr ) const
{
    return data_.z( nr );
}


int SEGY::FileData::trcNr( int nr ) const
{
    return data_.trcNr( nr );
}


bool SEGY::FileData::isNull( int nr ) const
{
    return data_.isSelected( nr );
}


bool SEGY::FileData::isUsable( int nr ) const
{
    return data_.group( nr ) != 2;
}
