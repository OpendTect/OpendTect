#ifndef prestackstacker_h
#define prestackstacker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: prestackstacker.h,v 1.2 2007-10-30 16:53:35 cvskris Exp $
________________________________________________________________________


-*/

#include "prestackprocessor.h"
#include "multiid.h"


template <class T> class Interval;


namespace PreStack
{

class Stack : public Processor
{
public:

    static void			initClass();
    static Processor*		createFunc();
    static const char*		sName()			{ return "Stack"; }

 				Stack();
    				~Stack();

    const char*			name() const		{ return sName(); }
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
    int				totalNr() const;
    bool			doWork(int,int,int);

    BufferString		errmsg_;
    Interval<float>*		offsetrg_;
};


}; //namespace

#endif
