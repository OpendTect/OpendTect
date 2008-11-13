/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

static const char* rcsID = "$Id: segyfiledata.cc,v 1.3 2008-11-13 11:33:21 cvsbert Exp $";

#include "segyfiledata.h"
#include "iopar.h"
#include "datapointset.h"
#include "survinfo.h"


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
    Coord ret( data_.coord( nr ) );
    ret.x += data_.value(0,nr); ret.y += data_.value(1,nr);
    return ret;
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


void SEGY::FileData::add( const SEGY::TraceInfo& ti )
{
    DataPointSet::DataRow dr;
    dr.pos_.nr_ = ti.nr_;
    dr.pos_.z_ = ti.offset_;
    dr.setSel( ti.isnull_ );
    dr.setGroup( ti.isusable_ ? 1 : 2 );

    dr.pos_.set( ti.binid_, ti.coord_ );
    const Coord poscoord( dr.pos_.coord() );
    dr.data_ += (float)(ti.coord_.x - poscoord.x);
    dr.data_ += (float)(ti.coord_.y - poscoord.y);

    data_.addRow( dr );
}


void SEGY::FileData::addEnded()
{
    data_.dataChanged();
}


void SEGY::FileData::getReport( IOPar& iop ) const
{
    BufferString str( "Global info for '" ); str += fname_; str += "'";
    iop.add( "->->", str );
    iop.add( "Number of traces found", nrTraces() );
    iop.add( "Number of samples in file", trcsz_ );
    iop.add( "Start position in file", sampling_.start );
    iop.add( "Step position in file", sampling_.step );
    iop.addYN( "REV. 1", isrev1_ );
    if ( isrev1_ && nrstanzas_ > 0 )
	iop.add( "Number of REV.1 extra stanzas", nrstanzas_ );
}


SEGY::FileDataSet& SEGY::FileDataSet::operator =( const SEGY::FileDataSet& fds )
{
    if ( this != &fds )
    {
	deepErase( *this );
	deepCopy( *this, fds );
    }
    return *this;
}


bool SEGY::FileDataSet::toNext( SEGY::FileDataSet::TrcIdx& ti, bool nll,
				bool unu ) const
{
    if ( ti.filenr_ < 0 )
	{ ti.trcnr_ = -1; ti.filenr_= 0; }

    if ( isEmpty() || ti.filenr_ >= size() )
	{ ti.filenr_ = -1; ti.trcnr_ = 0; return false; }

    ti.trcnr_++;
    if ( ti.trcnr_ >= (*this)[ti.filenr_]->nrTraces() )
	{ ti.filenr_++; ti.trcnr_ = -1; return toNext( ti, nll, unu ); }

    if ( nll && unu )
	return true;

    const FileData& fd = *(*this)[ti.filenr_];
    if ( (!nll && fd.isNull(ti.trcnr_))
      || (!unu && !fd.isUsable(ti.trcnr_)) )
	return toNext( ti, nll, unu );

    return true;
}
