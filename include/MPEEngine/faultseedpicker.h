#ifndef faultseedpicker_h
#define faultseedpicker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: faultseedpicker.h,v 1.3 2006-03-30 15:55:45 cvsjaap Exp $
________________________________________________________________________

-*/

#include "emseedpicker.h"

namespace MPE
{

class FaultSeedPicker : public EMSeedPicker
{
public:
    			FaultSeedPicker(MPE::EMTracker&);
    			~FaultSeedPicker() {}

    bool		canSetSectionID() const;
    bool		setSectionID(const EM::SectionID& sid);
    EM::SectionID	getSectionID() const;

    bool		startSeedPick();

    bool		canAddSeed() const;
    bool		addSeed(const Coord3&);
    bool		canRemoveSeed() const;
    bool		removeSeed(const EM::PosID&);
    bool		reTrack();
    int			nrSeeds() const;

    int			isMinimumNrOfSeeds() const	{ return 2; }
    bool		stopSeedPick(bool iscancel=false);

    void		blockSeedPick(bool yn)		{ blockpicking = yn; }
    bool		isSeedPickBlocked() const	{ return blockpicking; }

   const char*		errMsg() const; 
protected:
    bool		sectionIsEmpty() const;
    RowCol		getNewSeedRc(const Coord3&) const;

    MPE::EMTracker&	tracker;

    BufferString	errmsg;
    RowCol		stickstep;
    RowCol		stickstart;
    bool		isactive;
    int			nrseeds;
    EM::SectionID	sectionid;
    bool		isrowstick;
    bool		didchecksupport;
    bool		blockpicking;

};

};

#endif
