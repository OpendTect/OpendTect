/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

static const char* rcsID = "$Id: segyfiledata.cc,v 1.2 2008-10-08 15:57:32 cvsbert Exp $";

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


void SEGY::FileData::add( const BinID& bid, const Coord& c, int nr, float offs,
			  bool isnull, bool isusable )
{
    DataPointSet::DataRow dr;
    dr.pos_.nr_ = nr;
    dr.pos_.z_ = offs;
    dr.setSel( isnull );
    dr.setGroup( isusable ? 1 : 2 );

    dr.pos_.set( bid, c );
    const Coord poscoord( dr.pos_.coord() );
    dr.data_ += (float)(c.x - poscoord.x);
    dr.data_ += (float)(c.y - poscoord.y);

    data_.addRow( dr );
}


void SEGY::FileData::addEnded()
{
    data_.dataChanged();
}


void SEGY::FileData::getReport( IOPar& iop ) const
{
    BufferString str( "Global info for '" ); str += fname_; str += "'";
    iop.add( "->", str );
    iop.set( "Number of traces found", nrTraces() );
    iop.set( "Number of samples in file", trcsz_ );
    iop.set( "Start position in file", sampling_.start );
    iop.set( "Step position in file", sampling_.step );
    iop.setYN( "REV. 1", isrev1_ );
    if ( isrev1_ && nrstanzas_ > 0 )
	iop.set( "Number of REV.1 extra stanzas", nrstanzas_ );
}
