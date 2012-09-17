#ifndef volprocsmoother_h
#define volprocsmoother_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2008
 RCS:		$Id: volprocsmoother.h,v 1.9 2011/08/26 08:24:52 cvskris Exp $
________________________________________________________________________

-*/

#include "multiid.h"
#include "samplingdata.h"
#include "volprocchain.h"

template <class T> class Smoother3D;

namespace VolProc
{
    
mClass Smoother : public Step
{
public:
			mDefaultFactoryInstantiation( VolProc::Step,
			    Smoother, "Smoother", sFactoryKeyword() );
    
    			~Smoother();
			Smoother();

    bool		needsInput() const { return true; }
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

    void		releaseData();
    bool		canInputAndOutputBeSame() const	{ return false; }
    bool		needsFullVolume() const 	{ return true; }

    Task*		createTask();

protected:
    static const char*		sKeyZStepout()		{ return "ZStepout"; }

    bool                        prepareComp(int)	{ return true; }

    Smoother3D<float>*		smoother_;
};

}; //namespace


#endif
