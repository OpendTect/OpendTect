/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: dztimporter.cc,v 1.4 2010/03/29 09:13:14 cvsbert Exp $";

#include "dztimporter.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "seisselectionimpl.h"
#include "strmprov.h"
#include "datachar.h"
#include "survinfo.h"
#include <iostream>

static const float cNanoFac = 1e-9;


#define mRdVal(v) strm.read( (char*)(&v), sizeof(v) )

DZT::FileHeader::FileHeader()
    : nsamp(0)
    , nrdef_(1,1)
    , cstart_(SI().minCoord(true))
    , cstep_(SI().crlDistance(),0)
{
}


bool DZT::FileHeader::getFrom( std::istream& strm, BufferString& emsg )
{
    // From 0, (u)shorts:
    mRdVal(tag); mRdVal(data); mRdVal(nsamp); mRdVal(bits); mRdVal(zero);
    // From 10, floats:
    mRdVal(sps); mRdVal(spm); mRdVal(mpm); mRdVal(position); mRdVal(range);
    // From 30, dates (4 bytes each):
    mRdVal(created); mRdVal(modified);
    // From 38, unsigned shorts:
    mRdVal(npass); mRdVal(rgain); mRdVal(nrgain); mRdVal(text); mRdVal(ntext);
    mRdVal(proc); mRdVal(nproc); mRdVal(nchan);
    // From 54, floats:
    mRdVal(epsr); mRdVal(top); mRdVal(depth);
    // From 66, 1 byte dtype and 31 bytes reserved
    char buf[32]; strm.read( buf, 32 ); dtype = buf[32];
    // From 98, 14 bytes antenna
    strm.read( antname, 14 );
    // From 112, the rest
    mRdVal(chanmask); strm.read( buf, 12 );
    mRdVal(chksum);

    emsg.setEmpty();
#define mRetFalse nsamp = 0; return false
    if ( nsamp < 1 )
	{ emsg = "Zero Nr of samples found."; mRetFalse; }
    if ( sps < 1 )
	{ emsg = "Zero scans per second found."; mRetFalse; }
    if ( range < 1 )
	{ emsg = "Zero trace length found."; mRetFalse; }
    if ( data < 128 )
	{ emsg.add("Invalid data offset found: ").add(data); mRetFalse; }

    // dtype cannot be trusted, it seems
    if ( bits < 32 )
    {
	if ( dtype %2 )
	    dtype = bits == 16 ? 3 : 1;
	else
	    dtype = bits == 16 ? 2 : 0;
    }

    strm.seekg( std::streampos(data), std::ios::beg );
    if ( !strm.good() )
	{ emsg = "Cannot read first trace."; mRetFalse; }

    return true;
}


void DZT::FileHeader::fillInfo( SeisTrcInfo& ti, int trcidx ) const
{
    ti.nr = traceNr( trcidx );
    ti.sampling.start = position;
    ti.sampling.step = ((float)range) / (nsamp-1);
    ti.sampling.scale( cNanoFac );
    ti.coord.x = cstart_.x + cstep_.x * trcidx;
    ti.coord.y = cstart_.y + cstep_.y * trcidx;
}


#define mStrm (*sd_.istrm)

DZT::Importer::Importer( const char* fnm, const IOObj& ioobj,
			 const LineKey& lk )
    : Executor("Importing DZT file")
    , nrdone_(0)
    , totalnr_(-1)
    , lk_(lk)
    , wrr_(0)
    , databuf_(0)
    , di_(DataCharacteristics())
    , trc_(*new SeisTrc)
    , sd_(*new StreamData(StreamProvider(fnm).makeIStream()))
    , zfac_(1)
{
    if ( !sd_.usable() )
	return;

    if ( !fh_.getFrom( mStrm, msg_ ) )
	return;

    DataCharacteristics dc( fh_.dtype < 9, fh_.dtype %2 );
    dc.setNrBytes( fh_.dtype > 1 ? (fh_.dtype > 3 ? 4 : 2) : 1 );
    di_.set( dc );

    trc_.data().reSize( fh_.nsamp );
    for ( int ichan=1; ichan<fh_.nchan; ichan++ )
	trc_.data().addComponent( fh_.nsamp, DataCharacteristics() );

    wrr_ = new SeisTrcWriter( &ioobj );
    Seis::RangeSelData* rsd = new Seis::RangeSelData;
    rsd->setIsAll( true );
    rsd->lineKey() = lk;
    wrr_ = new SeisTrcWriter( &ioobj );
    wrr_->setSelData( rsd );

    databuf_ = new char [ fh_.nrBytesPerTrace() ];
    msg_ = "Handling traces";
}


DZT::Importer::~Importer()
{
    closeAll();
    delete [] databuf_;
    delete &trc_;
    delete &sd_;
}


int DZT::Importer::closeAll()
{
    sd_.close();
    delete wrr_; wrr_ = 0;
    return Finished();
}


#define mErrRet(s) { msg_ = s; return ErrorOccurred(); }

int DZT::Importer::nextStep()
{
    if ( !fh_.isOK() )
    {
	if ( !sd_.usable() )
	    mErrRet("Cannot open input file")
	else
	    return ErrorOccurred();
    }

    const int trcbytes = fh_.nrBytesPerTrace();
    mStrm.read( databuf_, trcbytes );
    if ( mStrm.gcount() != trcbytes )
	return closeAll();

    fh_.fillInfo( trc_.info(), nrdone_ );
    trc_.info().sampling.scale( zfac_ );

    for ( int ichan=0; ichan<fh_.nchan; ichan++ )
    {
	for ( int isamp=0; isamp<fh_.nsamp; isamp++ )
	    trc_.set( isamp, di_.get(databuf_,isamp)+fh_.zero, ichan );
	float firstrealval = trc_.get( 2, ichan );
	trc_.set( 0, firstrealval, ichan );
	trc_.set( 1, firstrealval, ichan );
    }

    if ( !wrr_->put(trc_) )
	mErrRet(wrr_->errMsg())

    nrdone_++;
    return mStrm.good() ? MoreToDo() : closeAll();
}
