#ifndef volprocsmoother_h
#define volprocsmoother_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Feb 2008
 RCS:		$Id: volprocsmoother.h,v 1.4 2008-09-25 18:47:27 cvskris Exp $
________________________________________________________________________

-*/

#include "multiid.h"
#include "samplingdata.h"
#include "volprocchain.h"

template <class T> class Smoother3D;

namespace VolProc
{
    
class Smoother : public Step
{
public:
    static void		initClass();
    
    			~Smoother();
			Smoother(Chain&);

    const char*		type() const			{ return sKeyType(); }
    bool		needsInput(const HorSampling&) const { return true; }
    HorSampling		getInputHRg(const HorSampling&) const;
    StepInterval<int>	getInputZRg(const StepInterval<int>&) const;

    bool		setOperator(const char*,float param,
	    			    int inlsz,int crlsz,int zsz);
    			//!<Size is set in multiples of inl/crl/z-step from SI.
    int			inlSz() const;
    int			crlSz() const;
    int			zSz() const;
    const char*		getOperatorName() const;
    float		getOperatorParam() const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    
    static const char*	sKeyType()			{ return "Smoother"; }
    static const char*	sUserName()			{ return sKeyType(); }

    Task*		createTask();

protected:
    static Step*		create(Chain&);
    static const char*		sKeyZStepout()		{ return "ZStepout"; }

    bool                        prepareComp(int)	{ return true; }

    Smoother3D<float>*		smoother_;
};

}; //namespace


#endif
