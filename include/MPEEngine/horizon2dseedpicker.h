#ifndef horizon2dseedpicker_h
#define horizon2dseedpicker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: horizon2dseedpicker.h,v 1.1 2006-05-05 19:07:54 cvskris Exp $
________________________________________________________________________

-*/

#include "emseedpicker.h"

namespace Attrib { class Data2DHolder; }

namespace MPE
{

class Horizon2DSeedPicker : public EMSeedPicker
{
public:
    			Horizon2DSeedPicker(MPE::EMTracker&);
    			~Horizon2DSeedPicker();
    void		setLine(const MultiID& lineset,const char*linename,
	    			const Attrib::Data2DHolder*);

    bool		canSetSectionID() const;
    bool		setSectionID(const EM::SectionID&);
    EM::SectionID	getSectionID() const;

    bool		startSeedPick();
    bool		addSeed(const Coord3&);
    bool		canAddSeed() const		{ return true; }
    bool		removeSeed(const EM::PosID&, bool retrack);
    bool		canRemoveSeed() const		{ return true; }
    bool		reTrack();
    int			nrSeeds() const;
    int			isMinimumNrOfSeeds() const;

    bool		stopSeedPick(bool iscancel=false);

    static int		nrSeedConnectModes()		{ return 3; }
    static int		defaultSeedConMode();
    static const char*	seedConModeText(int mode, 
					bool abbrev=false); 

    int			getSeedConnectMode() const;
    void		setSeedConnectMode(int scm);
    void		blockSeedPick(bool yn);
    bool		isSeedPickBlocked() const;
    bool		doesModeUseVolume() const;
    bool		doesModeUseSetup() const;

protected:

    MPE::EMTracker&		tracker_;

    EM::SectionID		sectionid_;
    bool			didchecksupport_;
    int				lineid_;


    const Attrib::Data2DHolder* data2d_;
    BufferString		linename_;
    MultiID			lineset_;
};


};

#endif
