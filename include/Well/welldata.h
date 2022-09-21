#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"
#include "sharedobject.h"

#include "callback.h"
#include "odcommonenums.h"
#include "multiid.h"
#include "namedobj.h"
#include "position.h"
#include "sets.h"
#include "uistring.h"
#include "welld2tmodel.h"
#include "wellman.h"


namespace Well
{

class Track;
class Log;
class LogSet;
class Marker;
class MarkerSet;
class D2TModel;
class DisplayProperties;


/*!
\brief Information about a certain well.
*/

mExpClass(Well) Info : public NamedCallBacker
{ mODTextTranslationClass(Well::Info)
public:

			Info(const char* nm);
			~Info();

    enum InfoType	{ None, Name, UWID, WellType, TD, KB, GroundElev,
			  SurfCoord, SurfBinID, Operator, Field, County, State,
			  Province, Country };
			mDeclareEnumUtils(InfoType)

    enum DepthType	{ MD, TVD, TVDSS, TVDSD, TWT };
			mDeclareEnumUtils(DepthType)

    static const char*	sKeyTVD()	{ return "True Vertical Depth [TVD]"; }
    static const char*	sKeyMD()	{ return "Measured Depth [MD]"; }

    bool		isLoaded() const;
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    BufferString	uwid_;
    BufferString	oper_;
    BufferString	field_;
    BufferString	county_;
    BufferString	state_;
    BufferString	province_;
    BufferString	country_;
    BufferString	source_; //!< filename for OD storage
    OD::WellType	welltype_		= OD::UnknownWellType;

    Coord		surfacecoord_;
    float		replvel_;
    float		groundelev_		= mUdf(float);

    static const char*	sKeyDepthUnit();
    static const char*	sKeyUwid();
    static const char*	sKeyOper();
    static const char*	sKeyField();
    static const char*	sKeyCounty();
    static const char*	sKeyState();
    static const char*	sKeyProvince();
    static const char*	sKeyCountry();
    static const char*	sKeyCoord();
    static const char*	sKeyKBElev();
    static const char*	sKeyTD();
    static const char*	sKeyTVDSS();
    static const char*	sKeyReplVel();
    static const char*	sKeyGroundElev();
    static const char*	sKeyWellType();
    static int		legacyLogWidthFactor();

    static uiString	sUwid();
    static uiString	sOper();
    static uiString	sField();
    static uiString	sCounty();
    static uiString	sState();
    static uiString	sProvince();
    static uiString	sCountry();
    static uiString	sCoord();
    static uiString	sKBElev();
    static uiString	sTD();
    static uiString	sTVDSS();
    static uiString	sReplVel();
    static uiString	sGroundElev();
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
				Data(const char* nm=0);

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
    const D2TModel*		d2TModel() const	{ return d2tmodel_; }
    D2TModel*			d2TModel()		{ return d2tmodel_; }
    const D2TModel*		checkShotModel() const	{ return csmodel_; }
    D2TModel*			checkShotModel()	{ return csmodel_; }
    void			setD2TModel(D2TModel*);	//!< becomes mine
    void			setCheckShotModel(D2TModel*); //!< mine, too
    DisplayProperties&		displayProperties( bool for2d=false )
				    { return for2d ? disp2d_ : disp3d_; }
    const DisplayProperties&	displayProperties( bool for2d=false ) const
				    { return for2d ? disp2d_ : disp3d_; }
    uiString			getInfoString(Info::InfoType) const;

    void			setEmpty(); //!< removes everything

    void			levelToBeRemoved(CallBacker*);

    const Well::Log*		getLog(const char* lognm) const;
    Well::Log*			getLogForEdit(const char* lognm);
    const BufferStringSet&	storedLogNames() const	{ return lognms_; }

    bool			haveLogs() const;
    bool			haveMarkers() const;
    bool			haveD2TModel() const	{ return d2tmodel_; }
    bool			haveCheckShotModel() const { return csmodel_; }
    Well::LoadReqs		loadState() const;
    void			reloadLogNames() const;
    void			reloadLogNames(CallBacker*);

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
    D2TModel*			d2tmodel_;
    D2TModel*			csmodel_;
    MarkerSet&			markers_;
    DisplayProperties&		disp2d_;
    DisplayProperties&		disp3d_;
    mutable BufferStringSet	lognms_;
};

} // namespace Well
