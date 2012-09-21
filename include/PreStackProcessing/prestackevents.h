#ifndef prestackevents_h
#define prestackevents_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2007
 RCS:		$Id$
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "prestackprocessingmod.h"
#include "bufstringset.h"
#include "callback.h"
#include "color.h"
#include "multiid.h"
#include "multidimstorage.h"
#include "offsetazimuth.h"
#include "position.h"
#include "refcount.h"
#include "undo.h"
#include "valseriesevent.h"

class Executor;
class BinIDValueSet;
class OffsetAzimuth;
class HorSampling;
class Color;
class SeisTrcReader;

namespace EM { class Horizon3D; }

namespace Threads { class Mutex; }
namespace PreStack
{

class CDPGeometrySet;
class GatherEvents;
class VelocityPicks;

/*!A Event is a set of picks on an event on a single prestack gather. */

mClass(PreStackProcessing) Event
{
public:
    			Event(int sz,bool quality);
    			Event(const Event& b);
    			~Event();
    Event&		operator=(const Event&);
    void		setSize(int sz,bool quality);
    void		removePick(int);
    void		addPick();
    void		insertPick(int);
    int			indexOf(const OffsetAzimuth&) const;

    int			sz_;
    float*		pick_;	
    OffsetAzimuth*	offsetazimuth_;

    static unsigned char cBestQuality()		{ return 254; }
    static unsigned char cManPickQuality()	{ return 255; }
    static unsigned char cDefaultQuality()	{ return 0; }
    unsigned char*	pickquality_;
    			//255	= manually picked
			//0-254 = tracked

    unsigned char	quality_;
    short		horid_;
    VSEvent::Type	eventtype_;
};


/*!A EventSet is a set of Events on a single prestack gather. */
mClass(PreStackProcessing) EventSet
{ mRefCountImplWithDestructor(EventSet,virtual ~EventSet(), {});
public:
    			EventSet();
    			EventSet(const EventSet&);
    EventSet&		operator=(const EventSet&);

    int			indexOf(int horid) const;

    ObjectSet<Event>	events_;
    bool		ischanged_;

    friend 		class EventManager;
};


/*!A EventManager is a set of EventsSet on multiple prestack
   gathers, and are identified under the same MultiID. */

mClass(PreStackProcessing) EventManager : public CallBacker
{ mRefCountImpl(EventManager);
public:
    mStruct(PreStackProcessing) DipSource
    {
			DipSource();
	enum Type	{ None, Horizon, SteeringVolume };
			DeclareEnumUtils(Type);

	bool		operator==(const DipSource& b) const;

	Type		type_;
	MultiID		mid_;

	void		fill(BufferString&) const;
	bool		use(const char*);
    };
				EventManager();

    const TypeSet<int>&		getHorizonIDs() const { return horids_; }
    int				addHorizon(int id=-1);
    				//!<\returns horid
				//!<\note id argument is only for internal use
				//!<	  i.e. the reader
    bool			removeHorizon(int id);
    const MultiID&		horizonEMReference(int id) const;
    void 			setHorizonEMReference(int id,const MultiID&);
    int				nextHorizonID(bool usethis);
    void			setNextHorizonID(int);

    const Color&		getColor() const	{ return color_; }
    void			setColor(const Color&);

    void			setDipSource(const DipSource&,bool primary);
    const DipSource&		getDipSource(bool primary) const;

    Executor*			setStorageID(const MultiID& mid,
	    				     bool reload );
    				/*!<Sets the storage id. 
				    \param reload if true,
					all data will be removed, forceReload
					will trigger, and data at the reload
					positions will be loaded from the new
					data.
				    Note that no data on disk is
				    changed/duplicated. That can be done with
				    the translator. */
    const MultiID&		getStorageID() const;

    bool			getHorRanges(HorSampling&) const;
    bool			getLocations(BinIDValueSet&) const;

    Undo&			undo()		{ return undo_; }
    const Undo&			undo() const	{ return undo_; }


    Executor*			commitChanges();
    Executor*			load(const BinIDValueSet&,bool trigger);

    bool			isChanged() const;
    void			resetChangedFlag(bool onlyhorflag);
    Notifier<EventManager>	resetChangeStatus;
    				//!<Triggers when the chang flags are reseted

    EventSet*			getEvents(const BinID&,bool load,bool create);
    const EventSet*		getEvents(const BinID&,
				    bool load=false,bool create=false) const;

    void			cleanUp(bool keepchanged);

    MultiDimStorage<EventSet*>&	getStorage() { return events_; }

    Notifier<EventManager>		change;
    					/*!<\note Dont enable/disable,
					    	  use blockChange if needed. */
    const BinID&			changeBid() const  { return changebid_;}
					/*!<Can be -1 if general
					   (name/color/horid) change. */
    void				blockChange(bool yn,bool sendall);
     					/*!<Turns off notification, but 
					    class will record changes. If
 					    sendall is on when turning off the
					    block, all recorded changes will
 					    be triggered. */

    Notifier<EventManager> 		forceReload;
    					/*!<When triggered, all
					    EventSets must be
					    unreffed. Eventual load requirements
					    should be added. */
    void				addReloadPositions(
	    						const BinIDValueSet&);
    void				addReloadPosition(const BinID&);

    void				reportChange(const BinID&);

    void				fillPar(IOPar&) const;
    bool				usePar(const IOPar&);


    bool				getDip(const BinIDValue&,int horid,
	    				       float& inldip, float& crldip );

protected:

    static const char*			sKeyStorageID() { return "PS Picks"; }
    bool				getDip(const BinIDValue&,int horid,
	    					bool primary,
						float& inldip, float& crldip );

    MultiDimStorage<EventSet*>	events_;
    Threads::Mutex&		eventmutex_;
    MultiID			storageid_;
    VelocityPicks*		velpicks_;

    Color			color_;

    TypeSet<int>		horids_;
    TypeSet<MultiID>		horrefs_;
    ObjectSet<EM::Horizon3D>	emhorizons_;

    BinID			changebid_;
    Threads::Mutex&		changebidmutex_;
    BinIDValueSet*		notificationqueue_;
    BinIDValueSet*		reloadbids_;

    int				nexthorid_;
    int				auxdatachanged_;

    DipSource			primarydipsource_;
    DipSource			secondarydipsource_;

    SeisTrcReader*		primarydipreader_;
    SeisTrcReader*		secondarydipreader_;

    Undo			undo_;
};


mClass(PreStackProcessing) SetPickUndo : public BinIDUndoEvent
{
public:
    			SetPickUndo(EventManager&,const BinID&,int horidx,
				    const OffsetAzimuth&,float depth,
				    unsigned char pickquality);
    const char*		getStandardDesc() const	{ return "prestack pick"; }
    const BinID&	getBinID() const	{ return bid_; }

    bool		unDo();
    bool		reDo();

protected:

    bool		doWork( float, unsigned char );

    EventManager&	manager_;
    const BinID		bid_;
    const int		horidx_;
    const OffsetAzimuth	oa_;
    const float		olddepth_;
    const unsigned char	oldquality_;
    float		newdepth_;
    unsigned char	newquality_;
};


mClass(PreStackProcessing) SetEventUndo : public UndoEvent
{
public:
    			SetEventUndo(EventManager&,const BinID&,int horidx,
				    short horid,VSEvent::Type,
				    unsigned char pickquality);
    			SetEventUndo(EventManager&,const BinID&,int horidx);
    const char*		getStandardDesc() const	{ return "prestack pick"; }
    const BinID&	getBinID() const	{ return bid_; }

    bool		unDo();
    bool		reDo();

protected:

    bool		addEvent();
    bool		removeEvent();

    EventManager&	manager_;
    const BinID		bid_;
    const int		horidx_;

    unsigned char	quality_;
    short		horid_;
    VSEvent::Type	eventtype_;

    bool		isremove_;
};


}; //namespace

#endif


