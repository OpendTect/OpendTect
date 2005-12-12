#ifndef horizonseedpicker_h
#define horizonseedpicker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: horizon3dseedpicker.h,v 1.1 2005-12-12 17:52:19 cvskris Exp $
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
};


};

#endif
