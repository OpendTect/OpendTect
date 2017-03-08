/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2007
-*/


#include "coltabindex.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "task.h"
#include "math2.h"


ColTab::IndexedLookUpTable::IndexedLookUpTable( const ColTab::Sequence& seq,
				    int nrc, const ColTab::Mapper& map )
    : seq_(seq)
    , mapper_(&map)
    , nrcols_(nrc)
    , mode_(map.setup().seqUseMode())
{
    update();
}


ColTab::IndexedLookUpTable::IndexedLookUpTable( const ColTab::Sequence& seq,
				    int nrc, ColTab::SeqUseMode mode )
    : seq_(seq)
    , mapper_(0)
    , nrcols_(nrc)
    , mode_(mode)
{
    update();
}


void ColTab::IndexedLookUpTable::update()
{
    if ( mapper_ )
	mode_ = mapper_->setup().seqUseMode();
    cols_.setSize( nrcols_, seq_.undefColor() );

    mDefParallelCalc3Pars( ColTabIndexAppl, tr("Update of color lookup table"),
	       TypeSet<Color>&,cols, const Sequence&,seq, SeqUseMode,mode )
    mDefParallelCalcBody(
,
	const float pos = ((float)idx) / ((float)(sz_-1));
	const float seqpos = Mapper::seqPos4RelPos( mode_, pos );
	cols_[(TypeSet<Color>::size_type)idx] = seq_.color( seqpos );
,
	)

    ColTabIndexAppl appl( cols_.size(), cols_, seq_, mode_ );
    appl.execute();
}


Color ColTab::IndexedLookUpTable::colorForIndex( int idx ) const
{
    if ( idx < 0 || idx >= nrcols_ )
	return seq_.undefColor();
    return cols_[idx];
}


int ColTab::IndexedLookUpTable::indexForValue( float v ) const
{
    return ColTab::Mapper::indexForValue( mapper_, v, nrcols_, -1 );
}
