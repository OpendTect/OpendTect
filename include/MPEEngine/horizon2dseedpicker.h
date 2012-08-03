#ifndef horizon2dseedpicker_h
#define horizon2dseedpicker_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: horizon2dseedpicker.h,v 1.14 2012-08-03 13:00:30 cvskris Exp $
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "emseedpicker.h"
#include "surv2dgeom.h"

namespace Attrib { class Data2DHolder; class SelSpec; }

namespace MPE
{

mClass(MPEEngine) Horizon2DSeedPicker : public EMSeedPicker
{
public:
    			Horizon2DSeedPicker(MPE::EMTracker&);
    			~Horizon2DSeedPicker()		{}
    void		setLine(const MultiID& lineset,const char*linename);

    bool		canSetSectionID() const;
    bool		setSectionID(const EM::SectionID&);
    EM::SectionID	getSectionID() const;

    bool		startSeedPick();
    bool		addSeed(const Coord3&,bool drop);
    bool		addSeed(const Coord3& seedcrd,bool drop,
				const Coord3& seedkey);
    bool		canAddSeed() const		{ return true; }
    bool		canAddSeed( const Attrib::SelSpec& );
    bool		removeSeed(const EM::PosID&, 
	    			   bool environment,
	    			   bool retrack);
    bool		canRemoveSeed() const		{ return true; }

    void		setSelSpec(const Attrib::SelSpec* selspec) 
			{ selspec_ = selspec; }
    const Attrib::SelSpec* getSelSpec()			{ return selspec_; }

    bool		reTrack();
    int			nrSeeds() const;
    int			minSeedsToLeaveInitStage() const;
    bool		stopSeedPick(bool iscancel=false);

    NotifierAccess*     aboutToAddRmSeedNotifier()      { return &addrmseed_; }
    NotifierAccess*     madeSurfChangeNotifier()        { return &surfchange_; }

    static int		nrSeedConnectModes()		{ return 3; }
    static int		defaultSeedConMode()		{return TrackFromSeeds;}
    static const char*  seedConModeText(int mode,
				    bool abbrev=false);

    int			getSeedConnectMode() const;
    void		setSeedConnectMode(int scm);
    void		blockSeedPick(bool yn);
    bool		isSeedPickBlocked() const;
    bool		doesModeUseVolume() const;
    bool		doesModeUseSetup() const;
    int                 defaultSeedConMode(
				    bool gotsetup) const;

    void		setSowerMode(bool yn)		{ sowermode_ = yn; }

protected:

    bool 			retrackOnActiveLine(int startcol,
						    bool startwasdefined,
						    bool eraseonly=false);

    void 			extendSeedListEraseInBetween(
				    bool wholeline, int startcol,
				    bool startwasdefined, int step);

    bool			retrackFromSeedList();
    int				nrLineNeighbors(int colnr) const;
    bool			interpolateSeeds();
    CubeSampling		getTrackBox() const;

    TypeSet<EM::PosID>		seedlist_;
    TypeSet<EM::PosID>		trackbounds_;
    TypeSet<EM::PosID>  	junctions_;
    TypeSet<EM::PosID>  	eraselist_;
    MPE::EMTracker&		tracker_;

    EM::PosID			lastseedpid_;
    Coord3			lastseedkey_;
    bool			sowermode_;

    const Attrib::SelSpec* 	selspec_;

    EM::SectionID		sectionid_;
    bool			didchecksupport_;
    PosInfo::GeomID		geomid_;

    BufferString		linename_;
    MultiID			lineset_;

    int				seedconmode_;
    bool			blockpicking_;

    Notifier<Horizon2DSeedPicker> 
				addrmseed_;
    Notifier<Horizon2DSeedPicker> 
				surfchange_;

};


};

#endif

