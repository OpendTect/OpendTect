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
#include "emseedpicker.h"
#include "posinfo2dsurv.h"
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

    void		setLine(Pos::GeomID);

    bool		startSeedPick();

    bool		addSeed(const TrcKeyValue&,bool drop);
    bool		addSeed(const TrcKeyValue& seedcrd,bool drop,
				const TrcKeyValue& seedkey);
    bool		canAddSeed( const Attrib::SelSpec& );
    bool		removeSeed(const TrcKey&,bool environment,bool retrack);
    TrcKey		replaceSeed(const TrcKey& oldpos,
				    const TrcKeyValue& newpos);

    bool		reTrack();

    bool		doesModeUseVolume() const;
    bool		updatePatchLine(bool);

protected:

    bool			retrackOnActiveLine(int startcol,
						    bool startwasdefined,
						    bool eraseonly=false);

    void			extendSeedSetEraseInBetween(
				    bool wholeline,int startcol,
				    bool startwasdefined,int step);

    bool			retrackFromSeedSet();
    int				nrLineNeighbors(int colnr) const;
    bool			interpolateSeeds(bool manualnode=false);
    TrcKeyZSampling		getTrackBox() const;
    bool			getNextSeedPos(int seedpos,int dirstep,
					       int& nextseedpos) const;
    bool			addPatchSowingSeeds();

    Pos::GeomID			geomid_;
};

} // namespace MPE
