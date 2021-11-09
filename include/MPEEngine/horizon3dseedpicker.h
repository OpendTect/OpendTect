#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
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
				const TrcKeyValue& seedkey);
    bool		removeSeed(const TrcKey&,bool environment,bool retrack);
    TrcKey		replaceSeed(const TrcKey& oldpos,
				    const TrcKeyValue& newpos);

    bool		reTrack();
    bool		doesModeUseVolume() const;

    void		setFaultData( const FaultTrcDataProvider* data )
			{ fltdataprov_ = data; }
    bool		updatePatchLine(bool);

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

