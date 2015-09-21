#ifndef emseedpicker_h
#define emseedpicker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "callback.h"

#include "attribsel.h"
#include "emposid.h"
#include "position.h"
#include "sets.h"
#include "trckeysampling.h"
#include "trckeyvalue.h"
#include "uistring.h"

namespace MPE
{

class EMTracker;

/*!
\brief Handles adding of seeds and retracking of events based on new seeds. An
instance of the class is usually available from each EMTracker.
*/

mExpClass(MPEEngine) EMSeedPicker: public CallBacker
{ mODTextTranslationClass(EMSeedPicker)
public:
    virtual		~EMSeedPicker() {}


    virtual void	setSectionID(EM::SectionID);
    virtual EM::SectionID getSectionID() const;

    virtual bool	startSeedPick();
			/*!<Should be set when seedpicking is about to start. */
    virtual void	endPatch(bool);
    virtual bool	isPatchEnded() const;
    bool		stopSeedPick();

    bool		addSeed(const TrcKeyValue&,bool drop=false);
    virtual bool	addSeed(const TrcKeyValue& seedcrd,bool drop,
				const TrcKeyValue& seedkey)	{ return false;}
    TrcKeyValue		getAddedSeed() const;
    void		getSeeds(TypeSet<TrcKey>&) const;
    int			indexOf(const TrcKey&) const;

    virtual bool	removeSeed(const TrcKey&,
	    			   bool enviromment=true,
	    			   bool retrack=true)		{ return false;}
    virtual TrcKey	replaceSeed(const TrcKey&,const TrcKeyValue&)
			{ return TrcKey::udf(); }

    virtual void	setSelSpec(const Attrib::SelSpec*);
    virtual const Attrib::SelSpec* getSelSpec() const;
    virtual bool	reTrack()				{ return false;}
    virtual int		nrSeeds() const;

    void		blockSeedPick(bool);
    bool		isSeedPickBlocked() const;
    void		setSowerMode(bool);
    bool		getSowerMode() const;

    enum TrackMode	{ TrackFromSeeds, TrackBetweenSeeds, DrawBetweenSeeds };
    static int		nrTrackModes(bool is2d);
    static uiString	getTrackModeText(TrackMode,bool is2d);

    void		setTrackMode(TrackMode);
    TrackMode		getTrackMode() const;
    bool		doesModeUseVolume() const		{ return false;}

    virtual const char*	errMsg() const				{ return 0; }


    void		setSeedPickArea(const TrcKeySampling&);
    const TrcKeySampling& getSeedPickArea() const;

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
    bool		endpatch_;
    EM::SectionID	sectionid_;
    TrackMode		trackmode_;
    TrcKeySampling	seedpickarea_;
    bool		sowermode_;
};

} // namespace MPE

#endif
