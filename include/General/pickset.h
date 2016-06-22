#ifndef pickset_h
#define pickset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
 Contents:	PickSet base classes
________________________________________________________________________

-*/

#include "picklocation.h"
#include "enums.h"
#include "namedobj.h"
#include "trckey.h"
#include "sets.h"
#include "draw.h"
#include "iopar.h"
#include "integerid.h"
template <class T> class ODPolygon;


namespace Pick
{


/*!\brief Monitorable set of pick locations.

  A Pick::Set is either a loose bunch of locations, or a connected set of
  points: a polygon. Apart from this, a set may be labeled to be part of a
  'category', like 'ArrowAnnotations'.

  In the near future, each location will get its own unique ID, the LocID. For
  now, this ID is identical to the index in the list. This is not MT-safe. But
  current code assumes LocID == IdxType.

*/

mExpClass(General) Set	: public RefCount::Referenced
			, public NamedMonitorable
{
public:

    typedef TypeSet<Location>::size_type    size_type;
    typedef size_type			    IdxType;
    typedef IntegerID<IdxType>		    LocID;

			Set(const char* nm=0,const char* category=0);
			mDeclMonitorableAssignment(Set);

    bool		isPolygon() const;
    void		setIsPolygon(bool yn=true);
    void		setCategory(const char*);
    BufferString	type() const; //!< sKey::Polygon() or sKey::PickSet()
    BufferString	category() const;

    size_type		size() const;
    inline bool		isEmpty() const			    { return size()<1; }
    bool		validLocID(LocID) const;
    bool		validIdx(IdxType) const;
    LocID		locIDFor(IdxType) const;
    IdxType		idxFor(LocID) const;

    Location		get(LocID) const;
    Coord		getPos(LocID) const;
    double		getZ(LocID) const;
    Location		first() const;
    Location		getByIndex(IdxType) const;

    Set&		setEmpty();
    Set&		append(const Set&);
    Set&		set(LocID,const Location&);
    LocID		insertBefore(LocID,const Location&);
    LocID		add(const Location&);
    LocID		remove(LocID); //!< returns ID of loc taking its place
    Set&		setPos(LocID,const Coord&);
    Set&		setZ(LocID,double);
    Set&		setByIndex(IdxType,const Location&);

    bool		isMultiGeom() const;
    Pos::GeomID		firstGeomID() const;
    bool		has2D() const;
    bool		has3D() const;
    bool		hasOnly2D() const;
    bool		hasOnly3D() const;

    void		getPolygon(ODPolygon<double>&) const; // coords
    void		getPolygon(ODPolygon<float>&) const; // binids
    void		getLocations(TypeSet<Coord>&) const;
    float		getXYArea() const;
			//!<Only for closed polygons. Returns in m^2.
    LocID		find(const TrcKey&) const;
    LocID		nearestLocation(const Coord&) const;
    LocID		nearestLocation(const Coord3&,bool ignorez=false) const;
    bool		removeWithPolygon(const ODPolygon<double>&,
					  bool inside=true);

    mImplSimpleMonitoredGetSet(inline,pars,setPars,IOPar,pars_,0)
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		updateInPar(const char* ky,const char* val);

    mExpClass(General) Disp
    {
    public:
	enum Connection { None, Open, Close };
			mDeclareEnumUtils(Connection);
			Disp() : connect_(None) {}
	bool		operator ==( const Disp& oth ) const
			{ return connect_ == oth.connect_
			      && mkstyle_ == oth.mkstyle_; }

	Connection		connect_;	//!< connect picks in set order
	OD::MarkerStyle3D	mkstyle_;
    };
    mImplSimpleMonitoredGetSet(inline,getDisp,setDisp,Disp,disp_,cDispChange())
    mImplSimpleMonitoredGetSet(inline,connection,setConnection,
				Disp::Connection,disp_.connect_,cDispChange())
    mImplSimpleMonitoredGetSet(inline,markerStyle,setMarkerStyle,
				OD::MarkerStyle3D,disp_.mkstyle_,cDispChange())
    mImplSimpleMonitoredGetSet(inline,dispColor,setDispColor,
				Color,disp_.mkstyle_.color_,cDispChange())
    mImplSimpleMonitoredGetSet(inline,dispSize,setDispSize,
				int,disp_.mkstyle_.size_,cDispChange())

    static ChangeType	cDispChange()		{ return 2; }
    static ChangeType	cLocationInsert()	{ return 3; }
    static ChangeType	cLocationChange()	{ return 4; }
    static ChangeType	cLocationRemove()	{ return 5; }
    static const char*	sKeyMarkerType()	{ return "Marker Type"; }

    mDeclInstanceCreatedNotifierAccess(Set);
    static const Set&	emptySet()		{ return emptyset_; }
    static Set&		dummySet()		{ return dummyset_; }

protected:

			~Set();

    TypeSet<Location>	locs_;
    TypeSet<LocID>	locids_;
    Disp		disp_;
    IOPar		pars_;
    mutable Threads::Atomic<IdxType> curlocidnr_;
    static const Set	emptyset_;
    static Set		dummyset_;

    IdxType		gtIdxFor(LocID) const;
    LocID		insNewLocID(IdxType,AccessLockHandler&);
    void		replaceID(LocID from,LocID to);

    friend class	SetIter;
    friend class	SetIter4Edit;
    friend class	SetManager; // for replaceID (undo/redo)

};


/*!\brief const Set iterator. Will MonitorLock, so when done before going out of
  scope, call retire().

  Needs a next() or prev() before a valid LocID is reached.

  */

mExpClass(General) SetIter
{
public:

			SetIter(const Set&,bool start_at_end=false);
			SetIter(const SetIter&);
			~SetIter()		{ retire(); }
    const Set&		pickSet() const		{ return *set_; }

    bool		next();
    bool		prev();

    bool		isValid() const;
    bool		atFirst() const	    { return curidx_ == 0; }
    bool		atLast() const;
    Set::LocID		ID() const;
    const Location&	get() const;
    Coord		getPos() const;
    double		getZ() const;

    void		retire();
    void		reInit(bool toend=false);

private:

    ConstRefMan<Set>	set_;
    Set::IdxType	curidx_;
    MonitorLock		ml_;

    SetIter&		operator =(const SetIter&); // pErrMsg

};


/*!\brief non-const Set iterator. Does not lock, so use this for non-shared
  Pick::Set's only. Really, because it locks totally nothing, many methods
  bypass the stand-alone Pick::Set's locking.

  Needs a next() or prev() before a valid LocID is reached.

  */

mExpClass(General) SetIter4Edit
{
public:

			SetIter4Edit(Set&,bool for_forward=true);
			SetIter4Edit(const SetIter4Edit&);
    SetIter4Edit&	operator =(const SetIter4Edit&);
    Set&		pickSet() const	 { return const_cast<Set&>(*set_); }

    bool		next();
    bool		prev();
    inline bool		advance( bool iterating_forward=true )
			{ return iterating_forward ? next() : prev(); }

    bool		isValid() const;
    bool		atFirst() const	    { return curidx_ == 0; }
    bool		atLast() const;
    Set::LocID		ID() const;
    Location&		get() const;
    void		removeCurrent(bool iterating_forward=true);
    void		insert(const Pick::Location&,bool iter_forward=true);

    void		reInit(bool for_forward=true);
    void		retire()	{}

private:

    RefMan<Set>		set_;
    Set::IdxType	curidx_;

};


} // namespace Pick


#endif
