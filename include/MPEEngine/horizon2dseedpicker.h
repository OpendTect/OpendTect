#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "emseedpicker.h"

#include "attribsel.h"
#include "trckeyzsampling.h"

namespace MPE
{

/*!
\brief EMSeedPicker to pick seeds in EM::Horizon2D.
*/

mExpClass(MPEEngine) Horizon2DSeedPicker : public EMSeedPicker
{ mODTextTranslationClass(Horizon2DSeedPicker)
public:
			Horizon2DSeedPicker(EMTracker&);
			~Horizon2DSeedPicker();

    void		setLine(const Pos::GeomID&);
    bool		startSeedPick() override;

    bool		addSeed(const TrcKeyValue&,bool drop);
    bool		addSeed(const TrcKeyValue& seedcrd,bool drop,
				const TrcKeyValue& seedkey) override;
    bool		canAddSeed( const Attrib::SelSpec& );
    bool		removeSeed(const TrcKey&,
				   bool environment,bool retrack) override;
    TrcKey		replaceSeed(const TrcKey& oldpos,
				    const TrcKeyValue& newpos) override;

    bool		reTrack() override;

    bool		doesModeUseVolume() const;
    bool		updatePatchLine(bool) override;

protected:

    bool			retrackOnActiveLine(int startcol,
						    bool startwasdefined,
						    bool eraseonly=false);

    void			extendSeedListEraseInBetween(
				    bool wholeline,int startcol,
				    bool startwasdefined,int step);

    bool			retrackFromSeedList();
    int				nrLineNeighbors(int colnr) const;
    bool			interpolateSeeds(bool manualnode);
    mDeprecatedDef
    bool			interpolateSeeds();

    TrcKeyZSampling		getTrackBox() const;
    bool			getNextSeedPos(int seedpos,int dirstep,
					       int& nextseedpos) const;
    bool			addPatchSowingSeeds();
    Pos::GeomID			geomid_;
};

} // namespace MPE
