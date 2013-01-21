#ifndef shiftattrib_h
#define shiftattrib_h

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
#include "position.h"

namespace Attrib
{

/*!
\ingroup Attributes
\brief %Shift Attribute
  %Shift takes the input at a specified position and outputs it at other
  relative positions.

<pre>
  %Shift pos= steering=Yes/No

  Input:
  0 - attrib to be hashed
  1 - steering (optional)
  
  %Output:
  0 - hashed attrib
</pre>
*/


mExpClass(Attributes) Shift : public Provider
{
public:
    static void			initClass();
				Shift(Desc&);

    static const char*		attribName()	{ return "Shift"; }
    static const char*		posStr()	{ return "pos"; }
    static const char*		timeStr()	{ return "time"; }
    static const char*		steeringStr()	{ return "steering"; }
    void			initSteering()	{ stdPrepSteering(stepout_); }

    void			prepPriorToBoundsCalc();

    virtual bool		isSingleTrace() const
				{ return !stepout_.inl && !stepout_.crl; }

protected:
    				~Shift() {}
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);
    void			init();

    bool			allowParallelComputation() const
    				{ return false; }

    bool			getInputOutput(int inp,TypeSet<int>& res) const;
    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const;

    const BinID*		reqStepout(int input,int output) const;
    const Interval<float>*	reqZMargin(int input,int output) const;
    const Interval<float>*	desZMargin(int input,int output) const;

    BinID			pos_;
    float 			time_;
    bool			dosteer_;
    
    BinID			stepout_;
    Interval<float>		interval_;
    Interval<float>		desinterval_;
    int				dataidx_;
    int				steeridx_;

    const DataHolder*		inputdata_;
    const DataHolder*		steeringdata_;
};

} // namespace Attrib


#endif

