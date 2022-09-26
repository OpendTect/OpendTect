#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "array2dbitmap.h"

/*!
\brief Common parameters for A2DBitMapGenerators.
*/

mStruct(General) WVAA2DBitMapGenPars : public A2DBitMapGenPars
{
		WVAA2DBitMapGenPars();
		~WVAA2DBitMapGenPars();

    bool	drawwiggles_;	//!< Draw the wiggles themselves
    bool	drawrefline_;	//!< Draw reference line for each trace
    bool	filllow_;	//!< Fill the left loops
    bool	fillhigh_;	//!< Fill the right loops
    bool	x1reversed_;	//!< If reversed, draw wiggles flipped

    float	overlap_;	//!< If > 0, part of the trace is drawn on
    				//!< both neighbours' display strip
    				//!< If < 0, uses less than entire strip
    float	reflinevalue_;
    int		minpixperdim0_;	//!< Set to 0 or neg for dump everything

    static char		cRefLineFill();		// => -126
    static char		cWiggFill();		// => -125
    static char		cLowFill();		// => -124
    static char		cHighFill();		// => -123
};


/*!
\brief Wiggles/Variable Area drawing on A2DBitMap.
*/

mExpClass(General) WVAA2DBitMapGenerator : public A2DBitMapGenerator
{
public:

			WVAA2DBitMapGenerator(const A2DBitMapInpData&,
					      const A2DBitMapPosSetup&);
			~WVAA2DBitMapGenerator();

    WVAA2DBitMapGenPars&	wvapars()		{ return gtPars(); }
    const WVAA2DBitMapGenPars&	wvapars() const		{ return gtPars(); }

protected:

    inline WVAA2DBitMapGenPars& gtPars() const
				{ return (WVAA2DBitMapGenPars&)pars_; }

    float			stripwidth_;

				WVAA2DBitMapGenerator(
					const WVAA2DBitMapGenerator&);
				//!< Not implemented to prevent usage
				//!< Copy the pars instead

    Interval<int>		getDispTrcIdxs() const;
    float			getDim0Offset(float val) const;
    int				dim0SubSampling(int nrdisptrcs) const;

    void			doFill() override;
    void			drawTrace(int);
    void			drawVal(int,int,float,float,float,float);
};


namespace Interpolate { template <class T> class Applier2D; }

/*!
\brief Variable density A2DBitMap generation parameters.
*/

mStruct(General) VDA2DBitMapGenPars : public A2DBitMapGenPars
{
			VDA2DBitMapGenPars();
			~VDA2DBitMapGenPars();

    bool		lininterp_;	//!< Use bi-linear interpol, not poly

    static char		cMinFill();	// => -120
    static char		cMaxFill();	// => 120

    static float	offset(char);	//!< cMinFill -> 0, 0 -> 0.5
};


/*!
\brief Variable density drawing on A2DBitMap.
*/

mExpClass(General) VDA2DBitMapGenerator :
			public A2DBitMapGenerator, ParallelTask
{
public:

			VDA2DBitMapGenerator(const A2DBitMapInpData&,
					     const A2DBitMapPosSetup&);
			VDA2DBitMapGenerator(
					const VDA2DBitMapGenerator&) = delete;
			//!< Copy the pars instead
			~VDA2DBitMapGenerator();

    VDA2DBitMapGenPars&		vdpars()	{ return gtPars(); }
    const VDA2DBitMapGenPars&	vdpars() const	{ return gtPars(); }

    void			linearInterpolate(bool yn)
				{ gtPars().lininterp_ = yn; }

protected:
    od_int64			nrIterations() const override;
    bool			doWork(od_int64,od_int64,int) override;

    inline VDA2DBitMapGenPars& gtPars() const
				{ return (VDA2DBitMapGenPars&)pars_; }

    float			strippixs_;

    void			doFill() override;

    void			drawStrip(int);
    void			drawPixLines(int,const Interval<int>&);
    void			fillInterpPars(Interpolate::Applier2D<float>&,
					       int,int);
    void			drawVal(int,int,float);

    TypeSet<int>		stripstodraw_;
};
