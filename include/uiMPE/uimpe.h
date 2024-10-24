#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimpemod.h"

#include "bufstring.h"
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
{
mODTextTranslationClass(uiTrackSettingsValidator)
public:
		uiTrackSettingsValidator();
		~uiTrackSettingsValidator();

    bool	checkInVolumeTrackMode() const override;
    bool	checkActiveTracker() const override;
    bool	checkStoredData(Attrib::SelSpec&,MultiID&) const override;
    bool	checkPreloadedData(const MultiID&) const override;
};

/*! Interface to track-setup groups */


mExpClass(uiMPE) uiSetupGroup : public uiGroup
{
mODTextTranslationClass(uiSetupGroup)
public:
			~uiSetupGroup();

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
    virtual void	updateAttribute()			{}

    virtual NotifierAccess*	modeChangeNotifier()	    { return nullptr; }
    virtual NotifierAccess*	propertyChangeNotifier()    { return nullptr; }
    virtual NotifierAccess*	eventChangeNotifier()	    { return nullptr; }
    virtual NotifierAccess*	correlationChangeNotifier() { return nullptr; }

    virtual bool	commitToTracker(bool& fieldchg) const   { return true; }
    virtual bool	commitToTracker() const;

    virtual void	showGroupOnTop(const char* grpnm)	{}
    virtual void	setMPEPartServer(uiMPEPartServer*)	{}

    BufferString	helpref_;

protected:
			uiSetupGroup(uiParent*,const char* helpref);
};


/*!
\brief Holder class for MPE ui-factories.
  Is normally only retrieved by MPE::uiMPE().
*/


mExpClass(uiMPE) uiMPEEngine final
{
public:
				uiMPEEngine();
				~uiMPEEngine();
};



/*!
\brief Access function for an instance (and normally the only instance) of
  MPE::uiMPEEngine.
*/
mGlobal(uiMPE) uiMPEEngine& uiMPE();

} // namespace MPE
