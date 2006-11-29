#ifndef array2dbitmapimpl_h
#define array2dbitmapimpl_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Sep 2006
 RCS:           $Id: array2dbitmapimpl.h,v 1.5 2006-11-29 18:09:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "array2dbitmap.h"


/*! \brief Common pars for A2DBitmapGenerators */

struct WVAA2DBitmapGenPars : public A2DBitmapGenPars
{
		WVAA2DBitmapGenPars()
		  : drawwiggles_(true)
		  , drawmid_(false)
		  , fillleft_(false)
		  , fillright_(true)
		  , midvalue_(0)
		  , overlap_(0.5)	{}

    bool	drawwiggles_;	//!< Draw the wiggles themselves
    bool	drawmid_;	//!< Draw mid line for each trace
    bool	fillleft_;	//!< Fill the left loops
    bool	fillright_;	//!< Fill the right loops

    float	overlap_;	//!< If > 0, part of the trace is drawn on
    				//!< both neighbours' display strip
    				//!< If < 0, uses less than entire strip
    float	midvalue_;	//!< if mUdf(float), use the median data value

    static const char	cZeroLineFill;		// => -126
    static const char	cWiggFill;		// => -125
    static const char	cLeftFill;		// => -124
    static const char	cRightFill;		// => -123

};


/*! \brief Wiggles/Variable Area Drawing on A2DBitmap's. */

class WVAA2DBitmapGenerator : public A2DBitmapGenerator
{
public:

			WVAA2DBitmapGenerator(const A2DBitMapInpData&,
					      const A2DBitmapPosSetup&);

    WVAA2DBitmapGenPars&	wvapars()		{ return gtPars(); }
    const WVAA2DBitmapGenPars&	wvapars() const		{ return gtPars(); }

protected:

    inline WVAA2DBitmapGenPars& gtPars() const
				{ return (WVAA2DBitmapGenPars&)pars_; }

    float			stripwidth_;

				WVAA2DBitmapGenerator(
					const WVAA2DBitmapGenerator&);
				//!< Not implemented to prevent usage
				//!< Copy the pars instead
    void			doFill();

    void			drawTrace(int);
    void			drawVal(int,int,float,float,float,float);

    bool			dumpXPM(std::ostream&) const;
};


namespace Interpolate { template <class T> class Applier2D; }


struct VDA2DBitmapGenPars : public A2DBitmapGenPars
{
			VDA2DBitmapGenPars()
			: lininterp_(false)	{}

    bool		lininterp_;	//!< Use bi-linear interpol, not poly

    static const char	cMinFill;	// => -120
    static const char	cMaxFill;	// => 120

    static float	offset(char);	//!< cMinFill -> 0, 0 -> 0.5

};


/*! \brief Wiggles/Variable Area Drawing on A2DBitmap's. */

class VDA2DBitmapGenerator : public A2DBitmapGenerator
{
public:

			VDA2DBitmapGenerator(const A2DBitMapInpData&,
					     const A2DBitmapPosSetup&);

    VDA2DBitmapGenPars&		vdpars()	{ return gtPars(); }
    const VDA2DBitmapGenPars&	vdpars() const	{ return gtPars(); }

protected:

    inline VDA2DBitmapGenPars& gtPars() const
				{ return (VDA2DBitmapGenPars&)pars_; }

    float			strippixs_;

				VDA2DBitmapGenerator(
					const VDA2DBitmapGenerator&);
				    //!< Not implemented to prevent usage
				    //!< Copy the pars instead

    void			doFill();

    void			drawStrip(int);
    void			drawPixLines(int,const Interval<int>&);
    void			fillInterpPars(Interpolate::Applier2D<float>&,
					       int,int);
    void			drawVal(int,int,float);

    bool			dumpXPM(std::ostream&) const;
};


#endif
