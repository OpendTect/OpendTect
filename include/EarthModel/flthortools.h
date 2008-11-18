#ifndef flthortools_h
#define flthortools_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		October 2008
 RCS:		$Id: flthortools.h,v 1.6 2008-11-18 14:37:18 jaap Exp $
________________________________________________________________________

-*/

#include "arrayndimpl.h"
#include "cubesampling.h"
#include "multiid.h"
#include "position.h"
#include "sets.h"

namespace EM { class FaultStickSet; class Horizon2D; }
class IOObj;

namespace SSIS
{

class FaultStickSubSampler
{
public:
    			FaultStickSubSampler(const EM::FaultStickSet&,
					     int sticknr,float zstep);
			~FaultStickSubSampler();

    bool		execute();

    Coord3		getCoord(float zval) const;
    const TypeSet<Coord3>& getCoordList() const;
    void		setZStep( float zs )	{ zstep_ = zs; }

protected:
    const EM::FaultStickSet& fault_;

    int			sticknr_;
    float		zstep_;
    TypeSet<Coord3>	crds_;
};


class FaultHorizon2DIntersectionFinder
{
public:
    		FaultHorizon2DIntersectionFinder(const EM::FaultStickSet&,
						 int sticknr,
						 const EM::Horizon2D&);
		~FaultHorizon2DIntersectionFinder();

    bool	find(float& trcnr,float& zval);

protected:

    const EM::FaultStickSet&	flt_;
    const EM::Horizon2D&	hor_;
    int				sticknr_;
};


class FaultHorizon2DLocationField : public Array2DImpl<char>
{
public:
    			FaultHorizon2DLocationField(const EM::FaultStickSet&,
						    int sticknr,
						    const EM::Horizon2D&,
						    const EM::Horizon2D&);
			~FaultHorizon2DLocationField();

    bool		calculate();

    static char		sOutside()		{ return '0'; }
    static char		sInsideNeg()		{ return '1'; }
    static char		sInsidePos()		{ return '2'; }

protected:
    CubeSampling		cs_;

    const EM::FaultStickSet&	flt_;
    const EM::Horizon2D&	tophor_;
    const EM::Horizon2D&	bothor_;

    int				sticknr_;
};



/*! \brief Calculates Throwfield between Fault and two horizons
*/

class FaultStickThrow
{
public:
			FaultStickThrow(const EM::FaultStickSet&,int sticknr,
				     const EM::Horizon2D&,const EM::Horizon2D&);
			~FaultStickThrow();

    float		getValue(float z,bool negtopos) const;


protected:

    bool		findInterSections(float&,float&);
    bool		init();

    const EM::FaultStickSet& flt_;

    const EM::Horizon2D& tophor_;
    const EM::Horizon2D& bothor_;

    int			sticknr_;

    float		topzneg_;
    float		topzpos_;
    float		botzneg_;
    float		botzpos_;
};

} // namespace SSIS


#endif
