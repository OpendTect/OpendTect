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

    bool		prepareWork();

    uiString		errMsg() const		{ return errmsg_; }

    static const char*	sTaperLength()		{ return "Taper Length";}
    static const char*	sTailMute()		{ return "Tail Mute";}
    static const char*	sMuteDef()		{ return "Mute Definition";}

    const DBKey&	muteDefID() const	{ return id_; }
    const MuteDef&	muteDef() const		{ return def_; }
    MuteDef&		muteDef()		{ return def_; }
    bool		isTailMute() const	{ return tail_; }
    float		taperLength() const	{ return taperlen_; }
    bool		setMuteDefID(const DBKey&);
    void		setEmptyMute();
    void		setTailMute(bool yn=true);
    void		setTaperLength(float);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    bool	 mustHaveUserInput() { return false; }

protected:

    MuteDef&		def_;
    Muter*		muter_;
    DBKey		id_;
    uiString		errmsg_;

    od_int64		nrIterations() const { return outidx_.size(); }
    bool		doWork(od_int64,od_int64,int);

    bool		tail_;
    float		taperlen_;

    TypeSet<int>	outidx_;
    TypeSet<int>	offsets_;
};

} // namespace PreStack
