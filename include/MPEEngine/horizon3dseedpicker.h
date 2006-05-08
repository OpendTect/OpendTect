#ifndef horizonseedpicker_h
#define horizonseedpicker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: horizon3dseedpicker.h,v 1.11 2006-05-08 13:41:58 cvsjaap Exp $
________________________________________________________________________

-*/

#include "emseedpicker.h"

namespace MPE
{

class HorizonSeedPicker : public EMSeedPicker
{
public:
    			HorizonSeedPicker(MPE::EMTracker&);

    bool		canSetSectionID() const		{ return true; }
    bool		setSectionID(const EM::SectionID&);
    EM::SectionID	getSectionID() const		{ return sectionid_; }

    bool		startSeedPick();
    bool		addSeed(const Coord3&);
    bool		canAddSeed() const		{ return true; }
    bool		removeSeed(const EM::PosID&,
	    			   bool retrack);
    bool		canRemoveSeed() const		{ return true; }
    bool		reTrack();
    int			nrSeeds() const;
    int			minSeedsToLeaveInitStage() const;

    bool		stopSeedPick(bool iscancel=false);

    enum SeedModeOrder	{ TrackFromSeeds, 
			  TrackBetweenSeeds, 
			  DrawBetweenSeeds   };

    static int		nrSeedConnectModes()		{ return 3; }
    static int		defaultSeedConMode()		{return TrackFromSeeds;}
    static const char*	seedConModeText(int mode, 
					bool abbrev=false); 

    int			getSeedConnectMode() const	{ return seedconmode_; }
    void		setSeedConnectMode(int scm)	{ seedconmode_ = scm; }
    void		blockSeedPick(bool yn)		{ blockpicking_ = yn; }
    bool		isSeedPickBlocked() const	{ return blockpicking_;}
    bool		doesModeUseVolume() const;
    bool		doesModeUseSetup() const;

protected:
    bool		retrackOnActiveLine( BinID startbid, 
					     bool startwasdefined,
					     bool eraseonly=false);
    bool		retrackFromSeedList();
    int			nrLateralConnections( const EM::PosID& ) const;
    bool		interpolateSeeds();
    CubeSampling	getTrackBox() const;

    TypeSet<EM::PosID>	seedlist_;
    TypeSet<Coord3>	seedpos_;
    TypeSet<BinID>	trackbounds_;
    bool		didchecksupport_;
    EM::SectionID	sectionid_;
    MPE::EMTracker&	tracker_;

    int			seedconmode_;
    bool		blockpicking_;
private: 
    void		extendSeedListEraseInBetween( 
			    bool wholeline,const BinID& startbid,
			    bool startwasdefined,const BinID& dir,
			    TypeSet<EM::PosID>& candidatejunctionpairs);
};


};

#endif
