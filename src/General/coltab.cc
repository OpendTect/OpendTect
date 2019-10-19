/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2007 / Mar 2017
-*/


#include "coltabsequence.h"
#include "coltabmapper.h"
#include "settings.h"
#include "paralleltask.h"


namespace ColTab
{

static ClipRatePair	defcliprate_ = ClipRatePair( mUdf(float), mUdf(float) );
static int		defhisteq_ = -1;

static const char*	sKeyDefClipPerc
			= "dTect.Disp.Default clip perc";
static const char*	sKeyDefHistEq
			= "dTect.Disp.Default histogram equalisation";


class TableFiller : public ParallelTask
{
public:

TableFiller( Table& tbl, const Sequence& seq, const Mapper& mapper )
    : tbl_(tbl)
    , seq_(seq)
    , mapper_(mapper)
{
}

virtual od_int64 nrIterations() const
{
    return tbl_.size();
}

virtual bool doWork( od_int64 startidx, od_int64 stopidx, int )
{
    for ( int idx=(int)startidx; idx<=(int)stopidx; idx++ )
	tbl_.setColor( seq_, mapper_, idx );
    return true;
}

    Table&	    tbl_;
    const Sequence& seq_;
    const Mapper&   mapper_;

};


class IndexTableFiller : public ParallelTask
{
public:

IndexTableFiller( IndexTable& tbl, const Mapper& mapper )
    : tbl_(tbl)
    , mapper_(mapper)
{
}

virtual od_int64 nrIterations() const
{
    return tbl_.size();
}

virtual bool doWork( od_int64 startidx, od_int64 stopidx, int )
{
    for ( int idx=(int)startidx; idx<=(int)stopidx; idx++ )
	tbl_.setIndex( mapper_, idx );
    return true;
}

    IndexTable&	    tbl_;
    const Mapper&   mapper_;

};

} // namespace ColTab



bool ColTab::isFlipped( SeqUseMode mode )
{
    return mode == FlippedSingle || mode == FlippedCyclic;
}


bool ColTab::isCyclic( SeqUseMode mode )
{
    return mode == UnflippedCyclic || mode == FlippedCyclic;
}


ColTab::SeqUseMode ColTab::getSeqUseMode( bool flipped, bool cyclic )
{
    return flipped ? (cyclic ? FlippedCyclic : FlippedSingle)
		   : (cyclic ? UnflippedCyclic : UnflippedSingle);
}


BufferString ColTab::toString( SeqUseMode mode )
{
    switch ( mode )
    {
    case UnflippedCyclic:	return BufferString( "Cyclic" );
    case FlippedSingle:		return BufferString( "Flipped" );
    case FlippedCyclic:		return BufferString( "ReverseCyclic" );
    case UnflippedSingle:	break;
    }
    return BufferString( "Normal" );
}


void ColTab::toPar( SeqUseMode mode, IOPar& iop )
{
    iop.set( sKeySeqUseMode(), toString(mode) );
}


bool ColTab::fromPar( const IOPar& iop, SeqUseMode& mode )
{
    BufferString modestr;
    if ( !iop.get(sKeySeqUseMode(),modestr) )
	return false;

    const char firstchar = modestr.firstChar();
    mode = UnflippedSingle;
    if ( firstchar == 'C' )
	mode = UnflippedCyclic;
    else if ( firstchar == 'F' )
	mode = FlippedSingle;
    else if ( firstchar == 'R' )
	mode = FlippedCyclic;

    return true;
}


void ColTab::convToPerc( ClipRatePair& crp )
{
    if ( !mIsUdf(crp.first()) )
	crp.first() *= 100.f;
    if ( !mIsUdf(crp.second()) )
	crp.second() *= 100.f;
}


void ColTab::convFromPerc( ClipRatePair& crp )
{
    if ( !mIsUdf(crp.first()) )
	crp.first() *= 0.01f;
    if ( !mIsUdf(crp.second()) )
	crp.second() *= 0.01f;
}


ColTab::ClipRatePair ColTab::defClipRate()
{
    if ( mIsUdf(defcliprate_.first()) || mIsUdf(defcliprate_.second()))
    {
	ClipRatePair clipperc( mUdf(float), mUdf(float) );
	Settings::common().get( sKeyDefClipPerc, clipperc );

	if ( mIsUdf(clipperc.first()) )
	    clipperc.first() = 2.5f;
	if ( mIsUdf(clipperc.second()) )
	    clipperc.second() = clipperc.first();
	convFromPerc( clipperc );
	defcliprate_ = clipperc;
    }

    return defcliprate_;
}


bool ColTab::defHistEq()
{
    if ( defhisteq_ < 0 )
    {
	bool yn = false;
	Settings::common().getYN( sKeyDefHistEq, yn );
	defhisteq_ = yn ? 1 : 0;
    }
    return defhisteq_;
}


void ColTab::setMapperDefaults( ClipRatePair clips, bool histeq )
{
    defcliprate_ = clips;
    convToPerc( clips );
    Settings::common().set( sKeyDefClipPerc, clips );
    Settings::common().setYN( sKeyDefHistEq, histeq );
    Settings::common().write();
}



ColTab::Table::Table( const SetType& cols, SamplingType sd )
    : cols_(cols)
    , sampling_(sd)
{
}


ColTab::Table::Table( const Sequence& seq, int tablesz, const Mapper& mpr )
{
    createTable( seq, mpr, tablesz );
}


ColTab::Table::Table( const Sequence& seq, int tablesz, SeqUseMode mode )
{
    RefMan<Mapper> mapper = new Mapper;
    mapper->setup().setSeqUseMode( mode );
    createTable( seq, *mapper, tablesz );
}


void ColTab::Table::setColor( const Sequence& seq, const Mapper& mapper,
				int idx )
{
    const ValueType calcval = sampling_.atIndex( idx );
    const PosType seqpos = mapper.seqPosition( calcval );
    cols_[idx] = seq.color( seqpos );
}


void ColTab::Table::createTable( const Sequence& seq, const Mapper& mapper,
				 int sz )
{
    cols_.setSize( sz, seq.undefColor() );
    Mapper::RangeType rg = mapper.getRange();
    sampling_.start = rg.start;
    if ( sz > 1 )
    {
	sampling_.step = rg.width() / (sz-1);
	sampling_.start = rg.start + sampling_.step * 0.5f;
	TableFiller filler( *this, seq, mapper );
	filler.execute();
    }
    else
    {
	if ( sz < 1 )
	    sampling_.step = 1.f;
	else
	{
	    sampling_.start = rg.center();
	    sampling_.step = (rg.stop - rg.start) * 0.5f;
	    if ( sampling_.step == 0.f )
		sampling_.step = 1.f;
	    setColor( seq, mapper, 0 );
	}
    }
}


int ColTab::Table::indexFor( ValueType val ) const
{
    const int idx = sampling_.nearestIndex( val );
    return getLimited( idx, 0, cols_.size()-1 );
}


Color ColTab::Table::colorAt( ValueType val ) const
{
    float fidx = sampling_.getfIndex( val );
    if ( fidx < 0.0001f )
	return cols_.first();
    else if ( fidx > cols_.size()-1.0001f )
	return cols_.last();

    const int idxbefore = (int)fidx;
    const int idxafter = idxbefore + 1;
    return Color::between( cols_[idxbefore], cols_[idxafter], fidx-idxbefore );
}



ColTab::IndexTable::IndexTable( const SetType& idxs, SamplingType sd )
    : idxs_(idxs)
    , sampling_(sd)
{
}


ColTab::IndexTable::IndexTable( int tablesz, const Mapper& mpr )
{
    createTable( mpr, tablesz );
}


ColTab::IndexTable::IndexTable( int tablesz, SeqUseMode mode )
{
    RefMan<Mapper> mapper = new Mapper;
    mapper->setup().setSeqUseMode( mode );
    createTable( *mapper, tablesz );
}


void ColTab::IndexTable::setIndex( const Mapper& mapper, int idx )
{
    const ValueType calcval = sampling_.atIndex( idx );
    idxs_[idx] = mapper.colIndex( calcval, idxs_.size() );
}


void ColTab::IndexTable::createTable( const Mapper& mapper, int sz )
{
    idxs_.setSize( sz, 0 );
    Mapper::RangeType rg = mapper.getRange();
    sampling_.start = rg.start;
    if ( sz > 1 )
    {
	sampling_.step = rg.width() / (sz-1);
	sampling_.start = rg.start + sampling_.step * 0.5f;
	IndexTableFiller filler( *this, mapper );
	filler.execute();
    }
    else
    {
	if ( sz < 1 )
	    sampling_.step = 1.f;
	else
	{
	    sampling_.start = rg.center();
	    sampling_.step = (rg.stop - rg.start) * 0.5f;
	    if ( sampling_.step == 0.f )
		sampling_.step = 1.f;
	    setIndex( mapper, 0 );
	}
    }
}


int ColTab::IndexTable::indexFor( ValueType val ) const
{
    int idx = sampling_.nearestIndex( val );
    if ( idx < 0 )
	idx = 0;
    else if ( idx > idxs_.size()-1 )
	idx = idxs_.size()-1;
    return idxs_[idx];
}
