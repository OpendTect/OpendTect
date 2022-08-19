#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volumeprocessing.h"
#include "multiid.h"

namespace VolProc
{

mClass(VolProcTest) ThresholdStep : public ProcessingStep
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
