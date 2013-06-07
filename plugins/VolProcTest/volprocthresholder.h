#ifndef volprocthresholder_h
#define volprocthresholder_h

/*+
_________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR		: Y.C. Liu
 DATE		: Mar 2007
 ID		: $Id: volprocthresholder.h,v 1.2 2009/07/22 16:01:27 cvsbert Exp $
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

