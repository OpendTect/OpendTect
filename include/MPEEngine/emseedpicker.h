#ifndef emseedpicker_h
#define emseedpicker_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "callback.h"
#include "emtracker.h"
#include "position.h"
#include "sets.h"

namespace Attrib { class SelSpec; }    

namespace MPE
{

/*!
handles adding of seeds and retracking of events based on new seeds.

An instance of the class is usually avaiable from the each EMTracker.
*/

mClass(MPEEngine) EMSeedPicker: public CallBacker
{
public:
    virtual		~EMSeedPicker() {}


    virtual bool	canSetSectionID() const			{ return false;}
    virtual bool	setSectionID(const EM::SectionID&)	{ return false;}
				
    virtual EM::SectionID getSectionID() const			{ return -1; }

    virtual bool	startSeedPick()				{ return false;}
    			/*!<Should be set when seedpicking
			    is about to start. */
    virtual bool	stopSeedPick(bool iscancel=false)	{ return true; }

    virtual bool	canAddSeed() const			{ return false;}
    virtual bool	addSeed(const Coord3&,bool drop=false)	{ return false;}
    virtual bool	addSeed(const Coord3& seedcrd,bool drop,
				const Coord3& seedkey)		{ return false;}
    virtual bool	canRemoveSeed() const			{ return false;}
    virtual bool	removeSeed(const EM::PosID&,
	    			   bool enviromment=true,
	    			   bool retrack=true)		{ return false;}
    virtual void	setSelSpec(const Attrib::SelSpec*) {}
    virtual const Attrib::SelSpec* 
			getSelSpec()				{ return 0; }
    virtual bool	reTrack()				{ return false;}
    virtual int		nrSeeds() const				{ return 0; }
    virtual int		minSeedsToLeaveInitStage() const	{ return 1; }

    virtual NotifierAccess* aboutToAddRmSeedNotifier()		{ return 0; }
    virtual NotifierAccess* madeSurfChangeNotifier()		{ return 0; }
    
    virtual void	setSeedConnectMode(int)			{ return; }
    virtual int		getSeedConnectMode() const		{ return -1; }
    virtual void	blockSeedPick(bool)			{ return; }
    virtual bool	isSeedPickBlocked() const		{ return false;}
    virtual bool        doesModeUseVolume() const		{ return true; }
    virtual bool	doesModeUseSetup() const		{ return true; }
    virtual int		defaultSeedConMode(bool gotsetup) const { return -1; }

    virtual const char*	errMsg() const				{ return 0; }

    enum SeedModeOrder  { TrackFromSeeds,
			TrackBetweenSeeds,
			DrawBetweenSeeds   };

    virtual void	setSowerMode(bool)			{};
    virtual void	setSeedPickArea(const HorSampling& hs)	{};

    virtual const HorSampling* getSeedPickArea() const		{ return 0; }
};


};

#endif

