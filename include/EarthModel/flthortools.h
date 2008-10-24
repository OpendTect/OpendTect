#ifndef flthortools_h
#define flthortools_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		October 2008
 RCS:		$Id: flthortools.h,v 1.1 2008-10-24 05:38:53 nanne Exp $
________________________________________________________________________

-*/

#include "arraynd.h"
#include "cubesampling.h"
#include "multiid.h"
#include "position.h"
#include "sets.h"

namespace EM { class Fault2D; class Horizon2D; }
class IOObj;

namespace SSIS
{

class Fault2DSubSampler
{
public:
    			Fault2DSubSampler(const EM::Fault2D&);
			~Fault2DSubSampler();

    void		getCoordList(float zstep,TypeSet<Coord3>&) const;

protected:
     const EM::Fault2D&	fault_;
};


class FaultHorizon2DIntersectionFinder
{
public:
    		FaultHorizon2DIntersectionFinder(const MultiID& fltid,
						 const MultiID& horid);

    void	setAreaOfInterest(const char* linenm)		{}
    bool	find(float& trcnr,float& zval);

protected:

    MultiID	fltid_;
    MultiID	horid_;
};


class FaultHorizon2DLocationField : public Array2D<char>
{
public:
    			FaultHorizon2DLocationField(const EM::Fault2D&,
				const EM::Horizon2D&,const EM::Horizon2D&,
				const IOObj& lsioobj,const char* linenm);
			~FaultHorizon2DLocationField();

    bool		calculate();

    static char		sOutside()		{ return '0'; }
    static char		sInsideNeg()		{ return '1'; }
    static char		sInsidePos()		{ return '2'; }

protected:
    CubeSampling	cs_;

    const EM::Fault2D&	fault_;
    const EM::Horizon2D& tophor_;
    const EM::Horizon2D& bothor_;

    const IOObj&	lsioobj_;
    BufferString	linename_;
};



} // namespace SSIS


#endif
