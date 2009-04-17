#ifndef faultseedpicker_h
#define faultseedpicker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: faultseedpicker.h,v 1.8 2009-04-17 06:52:28 cvsumesh Exp $
________________________________________________________________________

-*/

#include "emseedpicker.h"

namespace MPE
{

mClass FaultSeedPicker : public EMSeedPicker
{
public:
    			FaultSeedPicker(MPE::EMTracker&);
    			~FaultSeedPicker() {}

    bool		canSetSectionID() const;
    bool		setSectionID(const EM::SectionID& sid);
    EM::SectionID	getSectionID() const;

    bool		startSeedPick();

    bool		canAddSeed() const;
    bool		addSeed(const Coord3&,bool);
    bool		canRemoveSeed() const;
    bool		removeSeed(const EM::PosID&,bool environment,
	    			   bool retrack);

    bool		reTrack();
    int			nrSeeds() const;

    int			isMinimumNrOfSeeds() const	{ return 2; }
    bool		stopSeedPick(bool iscancel=false);

    enum FaultSeedConnectMode { DrawBetweenSeeds };
    static int		nrSeedConnectModes()	{ return 1; }
    static int		defaultSeedConMode()	{ return DrawBetweenSeeds; }
    static const char*	seedConModeText(int mode,bool abbrev=false);

    void		blockSeedPick(bool yn)		{ blockpicking_ = yn; }
    bool		isSeedPickBlocked() const	{ return blockpicking_;}

   const char*		errMsg() const; 
protected:
    bool		sectionIsEmpty() const;
    RowCol		getNewSeedRc(const Coord3&) const;

    MPE::EMTracker&	tracker_;

    BufferString	errmsg_;
    RowCol		stickstep_;
    RowCol		stickstart_;
    bool		isactive_;
    int			nrseeds_;
    EM::SectionID	sectionid_;
    bool		didchecksupport_;
    bool		blockpicking_;

};

} // namespace MPE

#endif
