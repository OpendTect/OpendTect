#ifndef volprocsmoother_h
#define volprocsmoother_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Feb 2008
 RCS:		$Id: volprocsmoother.h,v 1.1 2008-02-25 19:14:54 cvskris Exp $
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

    bool		setKernel(const char*,float param,
	    			  int sz0,int sz1,int sz2);

    bool 		setTopHorizon(const MultiID*,float tv);
    const MultiID*	getTopHorizonID() const;
    float		getTopValue() const;
    
    bool		setBottomHorizon(const MultiID*,float bv);	
    const MultiID*	getBottomHorizonID() const;
    float		getBottomValue() const;
    
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    
    static const char*	sKeyType()			{ return "Smoother"; }

    Task*		createTask();

protected:
    static Step*		create(Chain&);

    bool                        prepareComp(int)	{ return true; }

    Smoother3D<float>*		smoother_;
};

}; //namespace


#endif
