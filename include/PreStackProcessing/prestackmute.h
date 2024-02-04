#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessor.h"
#include "multiid.h"

class Muter;

namespace PreStack
{

class MuteDef;

/*!
\brief Processor for PreStack muting.
*/

mExpClass(PreStackProcessing) Mute : public Processor
{ mODTextTranslationClass(Mute)
public:
			mDefaultFactoryInstantiation(
				Processor, Mute,
				"Mute", toUiString(sFactoryKeyword()))

			Mute();
			~Mute();

    bool		prepareWork() override;

    uiString		errMsg() const override		{ return errmsg_; }

    static const char*	sTaperLength()		{ return "Taper Length";}
    static const char*	sTailMute()		{ return "Tail Mute";}
    static const char*	sMuteDef()		{ return "Mute Definition";}

    const MultiID&	muteDefID() const	{ return id_; }
    const MuteDef&	muteDef() const		{ return def_; }
    MuteDef&		muteDef()		{ return def_; }
    bool		isTailMute() const	{ return tail_; }
    float		taperLength() const	{ return taperlen_; }
    bool		setMuteDefID(const MultiID&);
    void		setEmptyMute();
    void		setTailMute(bool yn=true);
    void		setTaperLength(float);

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;
    bool		mustHaveUserInput() const override { return false; }

protected:

    MuteDef&		def_;
    Muter*		muter_ = nullptr;
    MultiID		id_;
    uiString		errmsg_;

    od_int64		nrIterations() const override { return outidx_.size(); }
    bool		doWork(od_int64,od_int64,int) override;

    bool		tail_ = false;
    float		taperlen_ = 10;

    TypeSet<int>	outidx_;
    TypeSet<int>	offsets_;
};

} // namespace PreStack
