#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2006
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "prestackprocessor.h"
#include "dbkey.h"


namespace PreStack
{

/*!
\brief Stacks PreStack data.
*/

mExpClass(PreStackProcessing) Stack : public Processor
{ mODTextTranslationClass(Stack);
public:
				mDefaultFactoryInstantiation( Processor, Stack,
				    "Stack", toUiString(sFactoryKeyword()))

				Stack();
				~Stack();

    uiString			errMsg() const		{ return errmsg_; }

    void			setOffsetRange(const Interval<float>*);
				//!<Null pointer means all offsets
    const Interval<float>*	getOffsetRange() const;
				//!<Null pointer means all offsets

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    static const char*		sKeyOffsetRange() { return "Offset Range"; }
    od_int64			nrIterations() const;
    bool			doWork(od_int64,od_int64,int);

    uiString			errmsg_;
    Interval<float>*		offsetrg_;
};

} // namespace PreStack
