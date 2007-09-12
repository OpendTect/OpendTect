#ifndef coltabmapper_h
#define coltabmapper_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: coltabmapper.h,v 1.2 2007-09-12 17:40:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "coltab.h"
#include "ranges.h"
#include "valseries.h"
class DataClipper;


namespace ColTab
{

/*!\brief Maps data values to color table positions: [0,1]

  If nrsegs_ > 0, the mapper will return the centers of the segments only. For
  example, if nsegs_ == 3, only positions returned are 1/6, 3/6 and 5/6.
 
 */

class Mapper
{
public:

    enum Type		{ Fixed, Auto, HistEq };

			Mapper();
			~Mapper();

    float		position(float val) const;
    			//!< returns position in ColorTable
    Interval<float>	range() const
			{ return Interval<float>( start_, start_ + width_ ); }
    const ValueSeries<float>* data() const
			{ return vs_; }
    int			dataSize() const
			{ return vssz_; }

    void		setRange( const Interval<float>& rg )
			{ start_ = rg.start; width_ = rg.stop - rg.start; }
    void		setData(const ValueSeries<float>*,int sz);
    			//!< If data changes, call update()

    void		update(bool full=true);
    			//!< If !full, will assume data is unchanged

    Type		type_;
    float		cliprate_;	// Auto
    float		symmidval_;	// Auto and HistEq. Usually mUdf(float)
    int			maxpts_;	// Auto and HistEq
    int			nrsegs_;	// All

protected:

    float		start_;
    float		width_;
    DataClipper&	clipper_;

    const ValueSeries<float>* vs_;
    int			vssz_;

};

} // namespace ColTab

#endif
