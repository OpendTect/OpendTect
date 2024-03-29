#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessor.h"
#include "multiid.h"


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

    uiString			errMsg() const override { return errmsg_; }

    void			setOffsetRange(const Interval<float>*);
				//!<Null pointer means all offsets
    const Interval<float>*	getOffsetRange() const;
				//!<Null pointer means all offsets

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

protected:
    static const char*		sKeyOffsetRange() { return "Offset Range"; }
    od_int64			nrIterations() const override;
    bool			doWork(od_int64,od_int64,int) override;

    uiString			errmsg_;
    Interval<float>*		offsetrg_ = nullptr;
};

} // namespace PreStack
