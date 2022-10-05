#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"

#include "factory.h"
#include "objectset.h"


namespace PreStack
{

class Gather;

/*!
\brief Base class for algorithms that compute semblance along a moveout.
*/

mExpClass(PreStackProcessing) SemblanceAlgorithm
{
public:
			mDefineFactoryInClass( SemblanceAlgorithm, factory );

    virtual		~SemblanceAlgorithm();
    virtual float	computeSemblance(const Gather&,
				      const float* moveout,
				      const bool* usetrace =nullptr) const = 0;
			/*!<Computes the semblance along the moveout.
			    \param moveout - One depth (absolute) per trace in
					     the gather, in the same order as
					     the gather.
			    \param usetrace  Optional array with one entry per
					     trace in the gather, int the same
					     order as the gather. If provided,
					     only traces with 'true' will be
					     included in computation. */

    virtual void	reInit()		{}
    virtual void	fillPar(IOPar&) const	{}
    virtual bool	usePar(const IOPar&)	{ return true; }

protected:
			SemblanceAlgorithm();
};

} // namespace PreStack
