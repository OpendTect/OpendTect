#ifndef coltabmapper_h
#define coltabmapper_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: coltabmapper.h,v 1.5 2008-09-23 10:40:08 cvshelene Exp $
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
struct MapperSetup {
		    MapperSetup();
    enum Type	{ Fixed, Auto, HistEq };


    mDefSetupClssMemb(MapperSetup,Type,type);
    mDefSetupClssMemb(MapperSetup,float,cliprate);	//!< Auto
    mDefSetupClssMemb(MapperSetup,float,symmidval);	//!< Auto and HistEq.
    							//!< Usually mUdf(float)
    mDefSetupClssMemb(MapperSetup,int,maxpts);		//!< Auto and HistEq
    mDefSetupClssMemb(MapperSetup,int,nrsegs);		//!< All
};


class Mapper
{
public:

			Mapper(); //!< defaults maps from [0,1] to [0,1]
			~Mapper();

    float		position(float val) const;
    			//!< returns position in ColorTable
    static int		snappedPosition(const Mapper*,float val,int nrsteps,
	    				int udfval);
    Interval<float>	range() const
			{ return Interval<float>( start_, start_ + width_ ); }
    const ValueSeries<float>* data() const
			{ return vs_; }
    int			dataSize() const
			{ return vssz_; }

    void		setRange( const Interval<float>& rg )
			{ start_ = rg.start; width_ = rg.stop - rg.start; }
    void		setData(const ValueSeries<float>*,od_int64 sz);
    			//!< If data changes, call update()

    void		update(bool full=true);
    			//!< If !full, will assume data is unchanged
			//
    MapperSetup		setup_;

protected:

    float		start_;
    float		width_;
    DataClipper&	clipper_;

    const ValueSeries<float>* vs_;
    od_int64		vssz_;

};

} // namespace ColTab

#endif
