#ifndef emseedpicker_h
#define emseedpicker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: emseedpicker.h,v 1.1 2005-09-19 06:58:43 cvskris Exp $
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

    bool		canSetSectionID() const { return false; }
    bool		setSectionID( const EM::SectionID& ) { return false; }
				
    EM::SectionID	getSectionID() const { return -1; }

    virtual bool	startSeedPick() { return false; }
    			/*!<Should be set when seedpicking is about 
			    to start. */

    virtual bool	canAddSeed() const { return false; }
    virtual bool	addSeed( const Coord3& ) { return false; }
    			/*!< */
    virtual bool	reTrack() { return false; }

    virtual bool	stopSeedPick() { return true; }
};


class HorizonSeedPicker : public EMSeedPicker
{
public:
    				HorizonSeedPicker( MPE::EMTracker& );

    bool			canSetSectionID() const { return true; }
    bool			setSectionID( const EM::SectionID& sid )
				    { sectionid = sid; return true; }
    EM::SectionID		getSectionID() const
				    { return sectionid; }

    bool			startSeedPick();
    bool			addSeed( const Coord3& );
    bool			canAddSeed() const { return true; }
    bool			reTrack();

    bool			stopSeedPick();

protected:
    TypeSet<EM::PosID>		seedlist;
    EM::SectionID		sectionid;
    int				firsthistorynr;
    MPE::EMTracker&		tracker;
};


};

#endif

