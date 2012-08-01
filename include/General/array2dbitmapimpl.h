#ifndef array2dbitmapimpl_h
#define array2dbitmapimpl_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Sep 2006
 RCS:           $Id: array2dbitmapimpl.h,v 1.16 2012-08-01 09:37:35 cvsmahant Exp $
________________________________________________________________________

-*/

#include "array2dbitmap.h"
#include "task.h"


/*! \brief Common pars for A2DBitMapGenerators */

mStruct WVAA2DBitMapGenPars : public A2DBitMapGenPars
{
		WVAA2DBitMapGenPars()
		  : drawwiggles_(true)
		  , drawmid_(false)
		  , fillleft_(false)
		  , fillright_(true)
		  , minpixperdim0_(2)
		  , overlap_(0.5)	{ midvalue_ = 0; }

    bool	drawwiggles_;	//!< Draw the wiggles themselves
    bool	drawmid_;	//!< Draw mid line for each trace
    bool	fillleft_;	//!< Fill the left loops
    bool	fillright_;	//!< Fill the right loops

    float	overlap_;	//!< If > 0, part of the trace is drawn on
    				//!< both neighbours' display strip
    				//!< If < 0, uses less than entire strip
    int		minpixperdim0_;	//!< Set to 0 or neg for dump everything

    static char		cZeroLineFill();		// => -126
    static char		cWiggFill();		// => -125
    static char		cLeftFill();		// => -124
    static char		cRightFill();		// => -123

};


/*! \brief Wiggles/Variable Area Drawing on A2DBitMap's. */

mClass WVAA2DBitMapGenerator : public A2DBitMapGenerator
{
public:

			WVAA2DBitMapGenerator(const A2DBitMapInpData&,
					      const A2DBitMapPosSetup&);

    WVAA2DBitMapGenPars&	wvapars()		{ return gtPars(); }
    const WVAA2DBitMapGenPars&	wvapars() const		{ return gtPars(); }

    int				dim0SubSampling() const;

protected:

    inline WVAA2DBitMapGenPars& gtPars() const
				{ return (WVAA2DBitMapGenPars&)pars_; }

    float			stripwidth_;

				WVAA2DBitMapGenerator(
					const WVAA2DBitMapGenerator&);
				//!< Not implemented to prevent usage
				//!< Copy the pars instead
    void			doFill();

    void			drawTrace(int);
    void			drawVal(int,int,float,float,float,float);

    bool			dumpXPM(std::ostream&) const;
};


namespace Interpolate { template <class T> class Applier2D; }


mStruct VDA2DBitMapGenPars : public A2DBitMapGenPars
{
			VDA2DBitMapGenPars()
			: lininterp_(false)	{}

    bool		lininterp_;	//!< Use bi-linear interpol, not poly

    static char		cMinFill();	// => -120
    static char		cMaxFill();	// => 120

    static float	offset(char);	//!< cMinFill -> 0, 0 -> 0.5

};


/*! \brief Wiggles/Variable Area Drawing on A2DBitMap's. */

mClass VDA2DBitMapGenerator : public A2DBitMapGenerator, ParallelTask
{
public:

			VDA2DBitMapGenerator(const A2DBitMapInpData&,
					     const A2DBitMapPosSetup&);

    VDA2DBitMapGenPars&		vdpars()	{ return gtPars(); }
    const VDA2DBitMapGenPars&	vdpars() const	{ return gtPars(); }
    
    void			linearInterpolate(bool yn)
				{ gtPars().lininterp_ = yn; }

protected:
    od_int64			nrIterations() const;
    bool			doWork(od_int64,od_int64,int);

    inline VDA2DBitMapGenPars& gtPars() const
				{ return (VDA2DBitMapGenPars&)pars_; }

    float			strippixs_;

				VDA2DBitMapGenerator(
					const VDA2DBitMapGenerator&);
				    //!< Not implemented to prevent usage
				    //!< Copy the pars instead

    void			doFill();

    void			drawStrip(int);
    void			drawPixLines(int,const Interval<int>&);
    void			fillInterpPars(Interpolate::Applier2D<float>&,
					       int,int);
    void			drawVal(int,int,float);

    bool			dumpXPM(std::ostream&) const;

    TypeSet<int>		stripstodraw_;
};


#endif
