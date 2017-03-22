#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		November 2010
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "factory.h"
#include "odmemory.h"
#include "dbkey.h"
#include "bufstring.h"

class Gather;

namespace PreStack
{
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
    virtual bool	setMute(bool inner,const DBKey&);

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

    DBKey			innermuteid_;
    DBKey			outermuteid_;

    uiString			errmsg_;
};


}; //namespace
