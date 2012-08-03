#ifndef prestackeventtracker_h
#define prestackeventtracker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		November 2010
 RCS:		$Id: prestackeventtracker.h,v 1.2 2012-08-03 13:00:33 cvskris Exp $
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

/*!Baseclass for algorithms that track pre-stack events on a gather. */

mClass(PreStackProcessing) EventTracker
{
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

    const char*		errMsg() { return errmsg_.str(); }

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

    BufferString		errmsg_;
};


}; //namespace

#endif

