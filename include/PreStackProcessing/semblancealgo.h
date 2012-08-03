#ifndef semblancealgo_h
#define semblancealgo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Nov 2010
 RCS:		$Id: semblancealgo.h,v 1.5 2012-08-03 13:00:34 cvskris Exp $
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "factory.h"
#include "objectset.h"

class IOPar;


namespace PreStack
{

class Gather;

/*! Base class for algorithms that computes semblance along a moveout */
mClass(PreStackProcessing) SemblanceAlgorithm
{
public:
    			mDefineFactoryInClass( SemblanceAlgorithm, factory );
    virtual		~SemblanceAlgorithm();
    virtual float	computeSemblance( const Gather&,
					  const float* moveout,
	   				  const bool* usetrace = 0 ) const = 0;
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
    virtual void	fillPar(IOPar&) const 	{}
    virtual bool	usePar(const IOPar&) 	{ return true; }
};

}; //namespace

#endif

