#ifndef prestackstacker_h
#define prestackstacker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: prestackstacker.h,v 1.9 2012-08-03 13:00:34 cvskris Exp $
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "prestackprocessor.h"
#include "multiid.h"


template <class T> class Interval;


namespace PreStack
{

mClass(PreStackProcessing) Stack : public Processor
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
    Gather*			createOutputArray(const Gather&) const;
    od_int64			nrIterations() const;
    bool			doWork(od_int64,od_int64,int);

    BufferString		errmsg_;
    Interval<float>*		offsetrg_;
};


}; //namespace

#endif

