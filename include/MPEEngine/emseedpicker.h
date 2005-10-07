#ifndef emseedpicker_h
#define emseedpicker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: emseedpicker.h,v 1.4 2005-10-07 19:17:29 cvskris Exp $
________________________________________________________________________

-*/

#include "emtracker.h"
#include "position.h"
#include "sets.h"

namespace MPE
{

/*!
handles adding of seeds and retracking of events based on new seeds.

An instance of the class is usually avaiable from the each EMTracker.
*/

class EMSeedPicker
{
public:
    virtual		~EMSeedPicker() {}

    virtual bool	canSetSectionID() const { return false; }
    virtual bool	setSectionID( const EM::SectionID& ) { return false; }
				
    virtual EM::SectionID getSectionID() const { return -1; }

    virtual bool	startSeedPick() { return false; }
    			/*!<Should be set when seedpicking is about 
			    to start. */

    virtual bool	canAddSeed() const { return false; }
    virtual bool	addSeed( const Coord3& ) { return false; }
    virtual bool	canRemoveSeed() const { return false; }
    virtual bool	removeSeed( const EM::PosID& ) { return false; }
    virtual bool	reTrack() { return false; }
    virtual int		nrSeeds() const { return 0; }

    virtual bool	stopSeedPick() { return true; }
};


class HorizonSeedPicker : public EMSeedPicker
{
public:
    			HorizonSeedPicker( MPE::EMTracker& );

    bool		canSetSectionID() const		{ return true; }
    bool		setSectionID( const EM::SectionID& sid );
    EM::SectionID	getSectionID() const		{ return sectionid; }

    bool		startSeedPick();
    bool		addSeed( const Coord3& );
    bool		canAddSeed() const		{ return true; }
    bool		removeSeed( const EM::PosID& );
    bool		canRemoveSeed() const		{ return true; }
    bool		reTrack();
    int			nrSeeds() const			{return seedpos.size();}

    bool		stopSeedPick();

protected:
    bool		removeEverythingButSeeds();

    TypeSet<EM::PosID>	seedlist;
    TypeSet<Coord3>	seedpos;
    int			firsthistorynr;
    bool		didchecksupport;

    EM::SectionID	sectionid;
    MPE::EMTracker&	tracker;
};

};

#endif
