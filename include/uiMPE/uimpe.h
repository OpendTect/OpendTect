#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimpemod.h"

#include "bufstring.h"
#include "callback.h"
#include "color.h"
#include "draw.h"
#include "emposid.h"
#include "emseedpicker.h"
#include "mpeengine.h"
#include "randcolor.h"
#include "valseriestracker.h"

#include "uigroup.h"

class TrcKeyValue;
class uiMPEPartServer;
class uiParent;

namespace Attrib { class DescSet; }


namespace MPE
{

class SectionTracker;


/*! Gui part of settings validator */
mExpClass(uiMPE) uiTrackSettingsValidator : public TrackSettingsValidator
{ mODTextTranslationClass(uiTrackSettingsValidator)
public:
    bool	checkInVolumeTrackMode() const override;
    bool	checkActiveTracker() const override;
    bool	checkStoredData(Attrib::SelSpec&,MultiID&) const override;
    bool	checkPreloadedData(const MultiID&) const override;
};

/*! Interface to track-setup groups. Implementations can be retrieved through
    MPE::uiSetupGroupFactory. */


mExpClass(uiMPE) uiSetupGroup : public uiGroup
{
public:
			uiSetupGroup(uiParent*,const char* helpref);
    virtual void	setSectionTracker(SectionTracker*)	{}
    virtual void	setMode(EMSeedPicker::TrackMode)	{}
    virtual EMSeedPicker::TrackMode getMode() const		= 0;
    virtual void	setTrackingMethod(EventTracker::CompareMethod)	{}
    virtual EventTracker::CompareMethod	getTrackingMethod() const	= 0;
    virtual void	setSeedPos(const TrcKeyValue&)		{}
    virtual void	setColor(const OD::Color&)		{}
    virtual const OD::Color& getColor()				= 0;
    virtual void	setLineWidth(int)			{}
    virtual int		getLineWidth() const			= 0;
    virtual void	setMarkerStyle(const MarkerStyle3D&)	{}
    virtual const MarkerStyle3D& getMarkerStyle()		= 0;

    virtual NotifierAccess*	modeChangeNotifier()		{ return 0; }
    virtual NotifierAccess*	propertyChangeNotifier()	{ return 0; }
    virtual NotifierAccess*	eventChangeNotifier()		{ return 0; }
    virtual NotifierAccess*	correlationChangeNotifier()	{ return 0; }

    virtual bool	commitToTracker(bool& fieldchg) const   { return true; }
    virtual bool	commitToTracker() const;

    virtual void	showGroupOnTop(const char* grpnm)	{}
    virtual void	setMPEPartServer(uiMPEPartServer*)	{}

    BufferString	helpref_;
};


/*! Factory function that can produce a MPE::uiSetupGroup* given a
    uiParent* and an Attrib::DescSet*. */

typedef uiSetupGroup*(*uiSetupGrpCreationFunc)(uiParent*,const char* typestr);

/*! Factory that is able to create MPE::uiSetupGroup* given a uiParent*,
    and an Attrib::DescSet*. Each class that wants to
    be able to procuce instances of itself must register itself with the
    addFactory startup. */

mExpClass(uiMPE) uiSetupGroupFactory
{
public:
    void		addFactory(uiSetupGrpCreationFunc f, const char* name);
    uiSetupGroup*	create(const char* nm,uiParent*,const char* typestr);
			/*!<Iterates through all added factory functions
			    until one of the returns a non-zero pointer. */
    void		remove(const char* nm);

protected:
    BufferStringSet			names_;
    TypeSet<uiSetupGrpCreationFunc>	funcs;

};


/*!
\brief Holder class for MPE ui-factories.
  Is normally only retrieved by MPE::uiMPE().
*/


mExpClass(uiMPE) uiMPEEngine
{
public:
    uiSetupGroupFactory		setupgrpfact;
};



/*!
\brief Access function for an instance (and normally the only instance) of
  MPE::uiMPEEngine.
*/
mGlobal(uiMPE) uiMPEEngine& uiMPE();

} // namespace MPE
