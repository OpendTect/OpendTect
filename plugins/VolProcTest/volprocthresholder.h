#ifndef volprocthresholder_h
#define volprocthresholder_h

/*+
_________________________________________________________________________

 COPYRIGHT:	(C) dGB Beheer B.V.
 AUTHOR		: Y.C. Liu
 DATE		: Mar 2007
 ID		: $Id: volprocthresholder.h,v 1.1 2007-03-30 21:00:56 cvsyuancheng Exp $
_________________________________________________________________________

-*/

#include "volumeprocessing.h"
#include "multiid.h"

namespace VolProc
{

class ThresholdStep : public ProcessingStep
{

public:
    static void		initClass();

    			ThresholdStep(ProcessingChain&);

    void		setThreshold(float);
    float		getThreshold() const;

    const char*		type() const;
    bool		needsInput(const HorSampling&) const;
    bool		compute(int start, int stop);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    
    
    static const char*	sKeyType()	{ return "Thresholder";}

    static const char*	sKeyThreshold() { return "Threshold value"; }
      
protected:

    static ProcessingStep*	create(ProcessingChain&);

    float			threshold_;
};


};
#endif

