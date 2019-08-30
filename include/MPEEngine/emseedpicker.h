#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          23-10-1996
 Contents:      Ranges
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "attribsel.h"
#include "emposid.h"
#include "position.h"
#include "trckeysampling.h"
#include "trckey.h"
#include "uistring.h"
#include "undo.h"


namespace MPE
{

class EMTracker;
class EMSeedPicker;
mExpClass(MPEEngine) Patch : public RefCount::Referenced
{
public:
			    Patch(const EMSeedPicker*);
    const TypeSet<TrcKeyValue>&  getPath() const;
    void		    getTrcKeySampling(TrcKeySampling&) const;
    int                     nrSeeds();
    Coord3                  seedCoord(int) const;
    int			    addSeed(const TrcKeyValue&,bool sort=true);
    void		    removeSeed(int);
    void		    clear();

protected:
                            ~Patch();
    EM::PosID               seedNode(int) const;
    int			    findClosestSeedRdmIdx(const EM::PosID&);
    int			    findClosedSeed3d(const EM::PosID&);
    int			    findClosedSeed2d(const TrcKeyValue&);
    const EMSeedPicker*     seedpicker_;
private:
    TypeSet<TrcKeyValue>    seeds_;
};
/*!
\brief Handles adding of seeds and retracking of events based on new seeds. An
instance of the class is usually available from each EMTracker.
*/

mExpClass(MPEEngine) EMSeedPicker: public CallBacker
{ mODTextTranslationClass(EMSeedPicker)
public:
    virtual		~EMSeedPicker();


    virtual bool	startSeedPick();
			/*!<Should be set when seedpicking is about to start. */

    const Patch*	getPatch() const { return patch_; }
    virtual void	endPatch(bool);
    bool		stopSeedPick();

    void		addSeedToPatch(const TrcKeyValue&,bool sort=true);
    bool		addSeed(const TrcKeyValue&,bool drop=false);
    virtual bool	addSeed(const TrcKeyValue& seedcrd,bool drop,
				const TrcKeyValue& seedkey)	{ return false;}
    TrcKeyValue		getAddedSeed() const;
    void		getSeeds(TypeSet<TrcKey>&) const;
    int			indexOf(const TrcKey&) const;

    virtual bool	removeSeed(const TrcKey&,
				   bool enviromment=true,
				   bool retrack=true)		{ return false;}
    virtual TrcKey	replaceSeed( const TrcKey&, const TrcKeyValue& )
			{ return TrcKey::udf(); }

    virtual void	setSelSpec(const Attrib::SelSpec*);
    virtual const Attrib::SelSpec* getSelSpec() const;
    virtual bool	reTrack()				{ return false;}
    virtual int		nrSeeds() const;

    void		blockSeedPick(bool);
    bool		isSeedPickBlocked() const;
    void		setSowerMode(bool);
    bool		getSowerMode() const;

    enum TrackMode	{ TrackFromSeeds, TrackBetweenSeeds,
			  DrawAndSnap, DrawBetweenSeeds };

    void		setTrackMode(TrackMode);
    TrackMode		getTrackMode() const;
    bool		doesModeUseVolume() const		{ return false;}

    virtual const uiString& errMsg() const   { return uiString::empty(); }


    void		setSeedPickArea(const TrcKeySampling&);
    const TrcKeySampling& getSeedPickArea() const;
    EMTracker&		emTracker() const { return tracker_; }
    bool		lineTrackDirection( BinID& dir,
				            bool perptotrackdir = false ) const;
    virtual bool	updatePatchLine(bool) { return false; }
    Undo&		horPatchUndo();
    const Undo&		horPatchUndo() const;
    bool		canUndo();
    bool		canReDo();


    Notifier<EMSeedPicker>	seedAdded;
    Notifier<EMSeedPicker>	seedRemoved;
    Notifier<EMSeedPicker>	seedToBeAddedRemoved;

protected:
			EMSeedPicker(EMTracker&);

    EMTracker&		tracker_;
    Attrib::SelSpec	selspec_;

    TrcKeyValue		addedseed_;
    TrcKeyValue		lastseed_;
    TrcKeyValue		lastsowseed_;

    TypeSet<TrcKey>	propagatelist_;
    TypeSet<TrcKey>	seedlist_;
    TypeSet<TrcKey>	trackbounds_;
    TypeSet<TrcKey>	junctions_;
    TypeSet<TrcKey>	eraselist_;

    bool		blockpicking_;
    bool		didchecksupport_;
    TrackMode		trackmode_;
    TrcKeySampling	seedpickarea_;
    bool		sowermode_;
    Patch*		patch_;
    Undo&		patchundo_;

};

} // namespace MPE
