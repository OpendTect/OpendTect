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
#include "uistring.h"

class FaultTrcDataProvider;
namespace Attrib { class SelSpec; }

namespace MPE
{

/*!
\brief SeedPicker to pick seeds in EM::Horizon3D.
*/

mExpClass(MPEEngine) Horizon3DSeedPicker : public EMSeedPicker
{ mODTextTranslationClass(Horizon3DSeedPicker)
public:
			Horizon3DSeedPicker(EMTracker&);
			~Horizon3DSeedPicker();

    bool		addSeed(const TrcKeyValue& seedcrd,bool drop,
				const TrcKeyValue& seedkey) override;
    bool		removeSeed(const TrcKey&,
				bool environment,bool retrack) override;
    TrcKey		replaceSeed(const TrcKey& oldpos,
				    const TrcKeyValue& newpos) override;

    bool		reTrack() override;
    bool		doesModeUseVolume() const;

    void		setFaultData( const FaultTrcDataProvider* data )
			{ fltdataprov_ = data; }
    bool		updatePatchLine(bool) override;

protected:
    bool		retrackOnActiveLine(const TrcKey& starttk,
					    bool startwasdefined,
					    bool eraseonly=false);
    bool		retrackFromSeedList();
    void		processJunctions();
    int			nrLateralNeighbors(const BinID& pid) const;
    int			nrLineNeighbors(const BinID& pid,
					bool perptotrackdir=false) const;

    bool		interpolateSeeds(bool setmanualnode);
    mDeprecatedDef
    bool		interpolateSeeds();

    TrcKeyZSampling	getTrackBox() const;
    bool		getNextSeed(const BinID& seedbid,const BinID& dir,
				    BinID& nextseedbid) const;
    bool		addPatchSowingSeeds();
    const FaultTrcDataProvider* fltdataprov_;

private:
    void		extendSeedListEraseInBetween(
			    bool wholeline,const TrcKey& starttk,
			    bool startwasdefined,const BinID& dir);
};

} // namespace MPE
