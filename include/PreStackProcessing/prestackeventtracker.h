#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"
#include "factory.h"
#include "odmemory.h"
#include "multiid.h"
#include "bufstring.h"


namespace PreStack
{
class Gather;
class Event;
class MuteDef;

/*!
\brief Base class for algorithms that track PreStack events on a gather.
*/

mExpClass(PreStackProcessing) EventTracker
{ mODTextTranslationClass(EventTracker)
public:
			mDefineFactoryInClass(EventTracker,factory);
    virtual void	reInit();
			//!<Should be called after each survey change

    virtual void	setMute(bool inner,MuteDef*,OD::PtrPolicy);
    virtual bool	setMute(bool inner,const MultiID&);

    virtual bool	trackEvents(const Gather&,ObjectSet<Event>&) const = 0;

    virtual		~EventTracker();
    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    uiString		errMsg() { return errmsg_; }

    static const char*	sKeyInnerMute() { return "Inner mute"; }
    static const char*	sKeyOuterMute() { return "Outer mute"; }

protected:
    				EventTracker();
    void			removeMutes();

    MuteDef*			innermute_;
    MuteDef*			outermute_;
    bool			ownsinnermute_;
    bool			ownsoutermute_;

    MultiID			innermuteid_;
    MultiID			outermuteid_;

    uiString			errmsg_;
};

} // namespace PreStack
