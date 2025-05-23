#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "coord.h"
#include "latlong.h"
#include "multiid.h"
#include "namedobj.h"
#include "odcommonenums.h"
#include "sharedobject.h"
#include "uistring.h"
#include "wellman.h"


class Mnemonic;

namespace Well
{

class D2TModel;
class DisplayProperties;
class Log;
class LogSet;
class Marker;
class MarkerSet;
class Track;


/*!
\brief Information about a certain well.
*/

mExpClass(Well) Info : public NamedCallBacker
{ mODTextTranslationClass(Well::Info)
public:
			Info(const char* nm);
			Info(const Info&);
			~Info();

    Info&		operator=(const Info&);

    enum InfoType	{ None, Name, UWID, WellType, TD, KB, GroundElev,
			  MarkerDepth, SurfCoord, SurfBinID, Operator, Field,
			  County, State, Province, Country };
			mDeclareEnumUtils(InfoType)

    enum DepthType	{ MD, TVD, TVDSS, TVDSD, TWT, TVDGL };
			mDeclareEnumUtils(DepthType)

    static const char*	sKeyTVD()	{ return "True Vertical Depth [TVD]"; }
    static const char*	sKeyMD()	{ return "Measured Depth [MD]"; }

    bool		isLoaded() const;
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    BufferString	uwid_;		// UWI
    BufferString	api_;		// API
    BufferString	oper_;		// SRVC - Service Company
    BufferString	field_;		// FLD - Field
    BufferString	county_;	// CNTY - County
    BufferString	state_;		// STAT - State
    BufferString	province_;	// PROV - Province
    BufferString	country_;	// CTRY - Country
    BufferString	company_;	// COMP - Company
    BufferString	license_;	// LIC - License number
    BufferString	date_;		// DATE
    BufferString	source_; //!< filename for OD storage
    OD::WellType	welltype_		= OD::UnknownWellType;

    Coord		surfacecoord_;
    LatLong		surfacelatlong_;
    float		replvel_;
    float		groundelev_		= mUdf(float);

    static const char*	sKeyDepthUnit();
    static const char*	sKeyDepthType();
    static const char*	sKeyUwid();
    static const char*	sKeyAPI();
    static const char*	sKeyOper();
    static const char*	sKeyField();
    static const char*	sKeyCounty();
    static const char*	sKeyState();
    static const char*	sKeyProvince();
    static const char*	sKeyCountry();
    static const char*	sKeyCompany();
    static const char*	sKeyLicense();
    static const char*	sKeyCoord();
    static const char*	sKeyKBElev();
    static const char*	sKeyTD();
    static const char*	sKeyTVDSS();
    static const char*	sKeyTVDSD();
    static const char*	sKeyTVDGL();
    static const char*	sKeyReplVel();
    static const char*	sKeyGroundElev();
    static const char*	sKeyMarkerDepth();
    static const char*	sKeyWellType();
    static int		legacyLogWidthFactor();

    static uiString	sUwid();
    static uiString	sAPI();
    static uiString	sOper();
    static uiString	sField();
    static uiString	sCounty();
    static uiString	sState();
    static uiString	sProvince();
    static uiString	sCountry();
    static uiString	sCompany();
    static uiString	sLicense();
    static uiString	sCoord();
    static uiString	sKBElev();
    static uiString	sTD();
    static uiString	sTVDSS();
    static uiString	sTVDSD();
    static uiString	sTVDGL();
    static uiString	sReplVel();
    static uiString	sGroundElev();

    static bool		isDepth(const Mnemonic&, DepthType&);
};


/*!
\brief The holder of all data concerning a certain well.

  For Well::Data from database this object gets filled with either calls to
  Well::MGR().get to get everything or Well::Reader::get*() to only get some
  of it (only track, only logs, ...).

  Note that a well is not a POSC well in the sense that it describes the data
  for one well bore. Thus, a well has a single track. This may mean duplication
  when more well tracks share an upper part.
*/

mExpClass(Well) Data : public SharedObject
{
public:
				Data(const char* nm=nullptr);

    const MultiID&		multiID() const		{ return mid_; }
    void			setMultiID( const MultiID& mid ) const
							{ mid_ = mid; }

    const OD::String&		name() const override	{ return info_.name(); }
    void			setName(const char* nm) override
				{ info_.setName( nm ); }
    const Info&			info() const		{ return info_; }
    Info&			info()			{ return info_; }
    const Track&		track() const		{ return track_; }
    Track&			track()			{ return track_; }
    const LogSet&		logs() const		{ return logs_; }
    LogSet&			logs()			{ return logs_; }
    const MarkerSet&		markers() const		{ return markers_; }
    MarkerSet&			markers()		{ return markers_; }
    int				nrD2TModels() const;
    void			getD2TModelNames(BufferStringSet&) const;
    int				nrCheckShotModels() const;
    void			getCheckShotModelNames(BufferStringSet&) const;
    const D2TModel*		d2TModel() const;
				//!< Active time-depth model
    const D2TModel*		d2TModelByName(const char*) const;
    const D2TModel*		d2TModelByIndex(int) const;
    D2TModel*			d2TModel();
    D2TModel*			d2TModelByName(const char*);
    D2TModel*			d2TModelByIndex(int);
    const D2TModel*		checkShotModel() const;
				//!< Active check-shot model
    const D2TModel*		checkShotModelByName(const char*) const;
    const D2TModel*		checkShotModelByIndex(int) const;
    D2TModel*			checkShotModel();
    D2TModel*			checkShotModelByName(const char*);
    D2TModel*			checkShotModelByIndex(int);
    bool			setD2TModel(D2TModel*);
				//!< becomes mine, replaces the active model
    bool			setCheckShotModel(D2TModel*);
				//!< mine, too, replaces the active model
    void			addD2TModel(D2TModel*,bool setasactive);
    void			addCheckShotModel(D2TModel*,bool setasactive);
    bool			setActiveD2TModel(D2TModel*);
    bool			setActiveCheckShotModel(D2TModel*);
    DisplayProperties&		displayProperties( bool for2d=false )
				    { return for2d ? disp2d_ : disp3d_; }
    const DisplayProperties&	displayProperties( bool for2d=false ) const
				    { return for2d ? disp2d_ : disp3d_; }
    uiString			getInfoString(Info::InfoType,
					const IOPar* modifier=nullptr) const;

    void			setEmpty(); //!< removes everything

    void			levelToBeRemoved(CallBacker*);

    const Well::Log*		getLog(const char* lognm) const;
    Well::Log*			getLogForEdit(const char* lognm);
    void			getLogNames(BufferStringSet&,
					    bool needreload=false) const;
    void			getLoadedLogNames(BufferStringSet&) const;

    bool			haveLogs() const;
    bool			haveMarkers() const;
    bool			haveD2TModel() const	{ return actd2tmodel_; }
    bool			haveCheckShotModel() const {return actcsmodel_;}
    Well::LoadReqs		loadState() const;
    void			reloadLogInfos() const;

    Notifier<Well::Data>	d2tchanged;
    Notifier<Well::Data>	csmdlchanged;
    Notifier<Well::Data>	markerschanged;
    Notifier<Well::Data>	trackchanged;
    Notifier<Well::Data>	disp3dparschanged;
    Notifier<Well::Data>	disp2dparschanged;
    CNotifier<Well::Data,int>	logschanged;
    Notifier<Well::Data>	reloaded;

protected:
    virtual			~Data();

    void			prepareForDelete() override;

    Info			info_;
    mutable MultiID		mid_;
    Track&			track_;
    LogSet&			logs_;
    D2TModel*			actd2tmodel_	= nullptr;
    D2TModel*			actcsmodel_	= nullptr;
    ObjectSet<D2TModel>		d2tmodels_;
    ObjectSet<D2TModel>		csmodels_;
    MarkerSet&			markers_;
    DisplayProperties&		disp2d_;
    DisplayProperties&		disp3d_;
};

} // namespace Well
