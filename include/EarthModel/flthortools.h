#ifndef flthortools_h
#define flthortools_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		October 2008
 RCS:		$Id: flthortools.h,v 1.9 2009-04-16 10:55:14 ranojay Exp $
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

mClass FaultStickSubSampler
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


mClass FaultHorizon2DIntersectionFinder
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


mClass FaultHorizon2DLocationField : public Array2DImpl<char>
{
public:
    			FaultHorizon2DLocationField(const EM::FaultStickSet&,
						    int sticknr,
						    const EM::Horizon2D&,
						    const EM::Horizon2D&);
			~FaultHorizon2DLocationField();

    bool		calculate();

    const char*		lineName() const	{ return linenm_; }
    const char		getPos(int trcnr,float z) const;
    const CubeSampling&	area() const		{ return cs_; }

    static char		sOutside()		{ return '0'; }
    static char		sInsideNeg()		{ return '1'; }
    static char		sInsidePos()		{ return '2'; }

protected:
    CubeSampling		cs_;

    const EM::FaultStickSet&	flt_;
    const EM::Horizon2D&	tophor_;
    const EM::Horizon2D&	bothor_;

    int				sticknr_;
    BufferString		linenm_;
};



/*! \brief Calculates Throwfield between Fault and two horizons
*/

mClass FaultStickThrow
{
public:
			FaultStickThrow(const EM::FaultStickSet&,int sticknr,
			     const EM::Horizon2D&,const EM::Horizon2D&);
			~FaultStickThrow();

    const char*		lineName() const	{ return linenm_; }
    float		getValue(float z,bool negtopos) const;


protected:

    bool		findInterSections(float&,float&);
    bool		init();

    const EM::FaultStickSet& flt_;

    const EM::Horizon2D& tophor_;
    const EM::Horizon2D& bothor_;

    int			sticknr_;
    BufferString	linenm_;

    float		topzneg_;
    float		topzpos_;
    float		botzneg_;
    float		botzpos_;
};

} // namespace SSIS


#endif
