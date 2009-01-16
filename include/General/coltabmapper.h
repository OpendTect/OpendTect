#ifndef coltabmapper_h
#define coltabmapper_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: coltabmapper.h,v 1.11 2009-01-16 04:49:32 cvsnanne Exp $
________________________________________________________________________

-*/

#include "enums.h"
#include "coltab.h"
#include "ranges.h"
#include "valseries.h"

class DataClipper;
class IOPar;

namespace ColTab
{

/*!\brief Maps data values to color table positions: [0,1]

  If nrsegs_ > 0, the mapper will return the centers of the segments only. For
  example, if nsegs_ == 3, only positions returned are 1/6, 3/6 and 5/6.
 
 */
struct MapperSetup : public CallBacker
{
			MapperSetup();
    enum Type		{ Fixed, Auto, HistEq };
    			DeclareEnumUtils(Type);

    mDefSetupClssMemb(MapperSetup,Type,type);
    mDefSetupClssMemb(MapperSetup,float,cliprate);	//!< Auto
    mDefSetupClssMemb(MapperSetup,bool,autosym0);	//!< Auto and HistEq.
    mDefSetupClssMemb(MapperSetup,float,symmidval);	//!< Auto and HistEq.
    							//!< Usually mUdf(float)
    mDefSetupClssMemb(MapperSetup,int,maxpts);		//!< Auto and HistEq
    mDefSetupClssMemb(MapperSetup,int,nrsegs);		//!< All
    mDefSetupClssMemb(MapperSetup,float,start);
    mDefSetupClssMemb(MapperSetup,float,width);

    bool 			operator==(const MapperSetup&) const;
    bool			operator!=(const MapperSetup&) const;
    MapperSetup&		operator=(const MapperSetup&);

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    static const char*		sKeyType()	{ return "Type"; }
    static const char*		sKeyClipRate()	{ return "Clip Rate"; }
    static const char*		sKeyAutoSym()	{ return "Auto Sym"; }
    static const char*		sKeySymMidVal()	{ return "Sym Mid Value"; }
    static const char*		sKeyMaxPts()	{ return "Max Pts"; }
    static const char*		sKeyRange()	{ return "Start_Width"; }

    void			triggerRangeChange();
    void			triggerAutoscaleChange();
    Notifier<MapperSetup>	rangeChange;
    Notifier<MapperSetup>	autoscaleChange;
};


mClass Mapper
{
public:

			Mapper(); //!< defaults maps from [0,1] to [0,1]
			~Mapper();

    float		position(float val) const;
    			//!< returns position in ColorTable
    static int		snappedPosition(const Mapper*,float val,int nrsteps,
	    				int udfval);
    Interval<float>	range() const;
    const ValueSeries<float>* data() const
			{ return vs_; }
    int			dataSize() const
			{ return vssz_; }

    void		setRange( const Interval<float>& rg );
    void		setData(const ValueSeries<float>*,od_int64 sz);
    			//!< If data changes, call update()

    void		update(bool full=true);
    			//!< If !full, will assume data is unchanged
			//
    MapperSetup		setup_;

protected:

    DataClipper&		clipper_;

    const ValueSeries<float>*	vs_;
    od_int64			vssz_;

};

} // namespace ColTab

#endif
