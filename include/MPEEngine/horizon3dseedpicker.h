#ifndef horizonseedpicker_h
#define horizonseedpicker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: horizon3dseedpicker.h,v 1.2 2006-01-25 14:51:59 cvsjaap Exp $
________________________________________________________________________

-*/

#include "emseedpicker.h"

namespace MPE
{

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

    bool		stopSeedPick(bool iscancel=false);

protected:
    bool		removeEverythingButSeeds();

    TypeSet<EM::PosID>	seedlist;
    TypeSet<Coord3>	seedpos;
    int			firsthistorynr;
    bool		didchecksupport;

    EM::SectionID	sectionid;
    MPE::EMTracker&	tracker;
    
    bool		interpolateSeeds();
    bool		interpolmode;
};


};

#endif
