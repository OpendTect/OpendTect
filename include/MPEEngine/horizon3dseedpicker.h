#ifndef horizonseedpicker_h
#define horizonseedpicker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: horizon3dseedpicker.h,v 1.5 2006-03-10 16:05:09 cvsjaap Exp $
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
    EM::SectionID	getSectionID() const		{ return sectionid_; }

    bool		startSeedPick();
    bool		addSeed( const Coord3& );
    bool		canAddSeed() const		{ return true; }
    bool		removeSeed( const EM::PosID& );
    bool		canRemoveSeed() const		{ return true; }
    bool		reTrack();
    int			nrSeeds() const;
    int			isMinimumNrOfSeeds() const;

    bool		stopSeedPick(bool iscancel=false);

    enum SeedModeOrder	{ TrackFromSeeds, TrackBetweenSeeds, DrawBetweenSeeds };
    int			getSeedMode() const		{ return seedmode_; }
    void		setSeedMode( int sm )		{ seedmode_ = sm; }
    void		freezeMode( bool yn )		{ frozen_ = yn; }
    bool		isModeFrozen() const		{ return frozen_; }
    bool		isInVolumeMode() const;

protected:
    bool		retrackActiveLine();
    void		repairDisconnections();
    bool		clearActiveLine();
    bool		interpolateSeeds();
    CubeSampling	getSeedBox() const;

    TypeSet<EM::PosID>	seedlist_;
    TypeSet<Coord3>	seedpos_;
    TypeSet<EM::PosID>	crosspid_;
    TypeSet<Coord3>	crosspos_;
    int			firsthistorynr_;
    bool		didchecksupport_;

    EM::SectionID	sectionid_;
    MPE::EMTracker&	tracker_;

    int			seedmode_;
    bool		frozen_;
};


};

#endif
