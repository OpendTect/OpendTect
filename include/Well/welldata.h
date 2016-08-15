#ifndef welldata_h
#define welldata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
________________________________________________________________________

-*/

#include "wellcommon.h"

#include "sharedobject.h"
#include "enums.h"
#include "multiid.h"
#include "position.h"
#include "notify.h"
#include "uistring.h"


namespace Well
{

class DisplayProperties;


/*!
\brief Information about a certain well.
*/

mExpClass(Well) Info : public ::NamedObject
{ mODTextTranslationClass(Well::Info)
public:

			Info( const char* nm )
			    : ::NamedObject(nm)
			    , replvel(Well::getDefaultVelocity())
			    , groundelev(mUdf(float))
			    , welltype_(None)
			{}

    enum WellType	{ None, Oil, Gas, OilGas, Dry, PluggedOil,
			  PluggedGas, PluggedOilGas, PermLoc, CancLoc,
			  InjectDispose };
			mDeclareEnumUtils(WellType);

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    BufferString	uwid;
    BufferString	oper;
    BufferString	state;
    BufferString	county;
    BufferString	source_; //!< filename for OD storage
    WellType		welltype_;

    Coord		surfacecoord;
    float		replvel;
    float		groundelev;

    static const char*	sKeyDepthUnit();
    static const char*	sKeyUwid();
    static const char*	sKeyOper();
    static const char*	sKeyState();
    static const char*	sKeyCounty();
    static const char*	sKeyCoord();
    static const char*	sKeyKBElev();
    static const char*	sKeyTD();
    static const char*	sKeyReplVel();
    static const char*	sKeyGroundElev();
    static const char*	sKeyWellType();
    static int		legacyLogWidthFactor();


    static uiString	sUwid();
    static uiString	sOper();
    static uiString	sState();
    static uiString	sCounty();
    static uiString	sCoord();
    static uiString	sKBElev();
    static uiString	sTD();
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
				mDeclInstanceCreatedNotifierAccess(Data);

    const MultiID&		multiID() const		{ return mid_; }
    void			setMultiID( const MultiID& mid ) const
				{ mid_ = mid; }

    virtual const OD::String&	name() const		{ return info_.name(); }
    virtual void		setName(const char* nm) { info_.setName( nm ); }
    virtual BufferString	getName() const	{ return info_.getName(); }

    const Info&			info() const		{ return info_; }
    Info&			info()			{ return info_; }

    const Track&		track() const		{ return track_; }
    Track&			track()			{ return track_; }

    const LogSet&		logs() const		{ return logs_; }
    LogSet&			logs()			{ return logs_; }

    const MarkerSet&		markers() const		{ return markers_; }
    MarkerSet&			markers()		{ return markers_; }

    const D2TModel&		d2TModel() const	{ return gtMdl(false); }
    D2TModel&			d2TModel()		{ return gtMdl(false); }
    const D2TModel&		checkShotModel() const	{ return gtMdl(true); }
    D2TModel&			checkShotModel()	{ return gtMdl(true); }

    DisplayProperties&		displayProperties( bool for2d=false )
				    { return for2d ? disp2d_ : disp3d_; }
    const DisplayProperties&	displayProperties( bool for2d=false ) const
				    { return for2d ? disp2d_ : disp3d_; }

    void			setEmpty(); //!< removes everything

    void			levelToBeRemoved(CallBacker*);

    bool			haveLogs() const;
    bool			haveMarkers() const;
    bool			haveD2TModel() const;
    bool			haveCheckShotModel() const;

    Notifier<Well::Data>	d2tchanged;
    Notifier<Well::Data>	csmdlchanged;
    Notifier<Well::Data>	markerschanged;
    Notifier<Well::Data>	trackchanged;
    Notifier<Well::Data>	disp3dparschanged;
    Notifier<Well::Data>	disp2dparschanged;
    CNotifier<Well::Data,int>	logschanged;
    Notifier<Well::Data>	reloaded;

				// Following return null when mdl is empty:
    D2TModel*			d2TModelPtr()	    { return gtMdlPtr(false); }
    D2TModel*			checkShotModelPtr() { return gtMdlPtr(true); }
    const D2TModel*		d2TModelPtr() const { return gtMdlPtr(false); }
    const D2TModel*		checkShotModelPtr() const
						    { return gtMdlPtr(true); }

protected:

			~Data();

    Info		info_;
    mutable MultiID	mid_;
    Track&		track_;
    LogSet&		logs_;
    D2TModel&		d2tmodel_;
    D2TModel&		csmodel_;
    MarkerSet&		markers_;
    DisplayProperties&	disp2d_;
    DisplayProperties&	disp3d_;

    D2TModel&		gtMdl(bool) const;
    D2TModel*		gtMdlPtr(bool) const;

};


mExpClass(Well) ManData
{
public:

			ManData(const MultiID&);

    bool		isAvailable() const;
    Well::Data&		data();
    const Well::Data&	data() const;

    MultiID		id_;

};


} // namespace Well

#endif
