#ifndef prestackstacker_h
#define prestackstacker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: prestackstacker.h,v 1.7 2009-07-22 16:01:17 cvsbert Exp $
________________________________________________________________________


-*/

#include "prestackprocessor.h"
#include "multiid.h"


template <class T> class Interval;


namespace PreStack
{

mClass Stack : public Processor
{
public:

    static void			initClass();
    static Processor*		createFunc();
    static const char*		sName()			{ return "Stack"; }

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
