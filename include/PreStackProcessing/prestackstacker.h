#ifndef prestackstacker_h
#define prestackstacker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "prestackprocessor.h"
#include "multiid.h"

template <class T> class Interval;

namespace PreStack
{

/*!
\brief Stacks PreStack data.
*/

mExpClass(PreStackProcessing) Stack : public Processor
{
public:
				mDefaultFactoryInstantiation( Processor, Stack,
					"Stack", sFactoryKeyword() );

 				Stack();
    				~Stack();

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    const char*			errMsg() const;

    void			setOffsetRange(const Interval<float>*);
				//!<Null pointer means all offsets
    const Interval<float>*	getOffsetRange() const;
				//!<Null pointer means all offsets

protected:
    static const char*		sKeyOffsetRange() { return "Offset Range"; }
    od_int64			nrIterations() const;
    bool			doWork(od_int64,od_int64,int);

    BufferString		errmsg_;
    Interval<float>*		offsetrg_;
};


}; //namespace

#endif

