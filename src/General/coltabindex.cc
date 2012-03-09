/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2007
-*/

static const char* rcsID = "$Id: coltabindex.cc,v 1.13 2012-03-09 17:55:20 cvsnanne Exp $";

#include "coltabindex.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "task.h"
#include "math2.h"


ColTab::IndexedLookUpTable::IndexedLookUpTable( const ColTab::Sequence& seq,
				    int nrc, const ColTab::Mapper* map )
    : seq_(seq)
    , mapper_(map)
    , nrcols_(nrc)
{
    update();
}


#define mParallelApplyToAll( vartype, obj, nriterimpl, preops, op, postops, \
				     resultop ) \
{ \
    class ApplyClass : public ParallelTask \
    { \
    public: \
		ApplyClass( vartype& var ) : self ( var ) {} \
	od_int64 nrIterations() const { return nriterimpl; } \
	bool	doWork( od_int64 start, od_int64 stop, int ) \
	{ \
	    preops; \
	    for ( int idx=start; idx<=stop; idx++ ) \
	    { \
		op; \
	    } \
	 \
	    postops; \
	    return true; \
	} \
    protected: \
      vartype&	self; \
    } applyinst( obj ); \
    resultop applyinst.execute(); \
}

void ColTab::IndexedLookUpTable::update()
{
    cols_.erase();
    cols_.setSize( nrcols_, seq_.undefColor() );
    mParallelApplyToAll( ColTab::IndexedLookUpTable, (*this),
	    self.cols_.size(),
	    const float dx = 1./(self.cols_.size()-1),
	    self.cols_[idx] = self.seq_.color( idx*dx ),
	    , )
}


Color ColTab::IndexedLookUpTable::colorForIndex( int idx ) const
{
    if ( idx < 0 || idx >= nrcols_ )
	return seq_.undefColor();
    return cols_[idx];
}


int ColTab::IndexedLookUpTable::indexForValue( float v ) const
{
    return ColTab::Mapper::snappedPosition( mapper_, v, nrcols_, -1 );
}
