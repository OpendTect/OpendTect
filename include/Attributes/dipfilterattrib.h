#ifndef dipfilterattrib_h
#define dipfilterattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________

-*/

#include "attributesmod.h"
#include "attribprovider.h"
#include "arrayndimpl.h"

namespace Attrib
{

/*!
\ingroup Attributes
\brief Dip filtering Attribute

  %DipFilter convolves a signal with the on the command-line specified signal.

  minvel/maxvel is the cutoff velocity (m/s) or dip (degrees), depending
  on the z-range unit.

  Azimuth is given in degrees from the inline, in the direction
  of increasing crossline-numbers.

  %Taper is given as percentage. Tapering is done differently for the three
  different filtertype.
  Bandpass tapers at the upper and lower taperlen/2 parts of t the interval.
  Lowpass tapers from the maxvel down to 100-taperlen of the maxvel.
  Highpass tapers from minvel to 100+taperlen of minvel

  The azimuthrange is tapered in the same way as bandpass.

<pre>
%DipFilter size= minvel= maxvel= type=LowPass|HighPass|BandPass
	   filterazi=Y/N minazi= maxazi= taperlen=

type = HighPass
          x     x minvel > 0
	  xx   xx
	  xxx xxx
	  xxxxxxx
	  xxx xxx
	  xx   xx
	  x     x

type = LowPass
          xxxxxxx maxvel > 0
           xxxxx
            xxx
             x
            xxx
           xxxxx
          xxxxxxx
								
type = BandPass
             x       x  minvel
            xxx     xxx
            xxxx   xxxx maxvel > 0
              xxx xxx
                 x
              xxx xxx
            xxxx   xxxx
            xxx     xxx
             x       x

Inputs:
0       Signal to be filtered.
</pre>
*/

mClass(Attributes) DipFilter : public Provider
{
public:
    static void		initClass();
			DipFilter(Desc&);

    static const char*	attribName()	{ return "DipFilter"; }
    static const char*	sizeStr()	{ return "size"; }
    static const char*	typeStr()	{ return "type"; }
    static const char*	minvelStr()	{ return "minvel"; }
    static const char*	maxvelStr()	{ return "maxvel"; }
    static const char*	filteraziStr()	{ return "filterazi"; }
    static const char*	minaziStr()	{ return "minazi"; }
    static const char*	maxaziStr()	{ return "maxazi"; }
    static const char*  taperlenStr()   { return "taperlen"; }
    static const char*	filterTypeNamesStr(int);

protected:
			~DipFilter() {}
    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);
    static void         updateDefaults(Desc&);

    bool		allowParallelComputation() const	{ return true; }

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
	    			    int t0,int nrsamples,int threadid) const;
    bool		initKernel();
    void		prepareForComputeData() { initKernel(); }
    float		taper(float) const;

    const BinID*		desStepout(int input,int output) const;
    const Interval<int>*	desZSampMargin(int,int) const;

    int				size_;
    int				type_;
    float			minvel_;
    float			maxvel_;
    bool			filterazi_;

    float			minazi_;
    float			maxazi_;
    float			taperlen_;
    bool			isinited_;

    Array3DImpl<float>  	kernel_;
    Interval<float>     	valrange_;
    float            	   	azi_;
    float               	aziaperture_;

    BinID               	stepout_;
    Interval<int>		zmargin_;
    int				dataidx_;

    ObjectSet<const DataHolder>	inputdata_;
};

}; // namespace Attrib


#endif


