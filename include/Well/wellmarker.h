#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellcommon.h"
#include "sharedobject.h"
#include "color.h"
#include "ranges.h"
#include "monitoriter.h"
#include "stratlevel.h"

class TaskRunner;



namespace Well
{


/*!\brief Marker, can be attached to Strat level. When not attached, uses
  the object's own (fallback) name and color. */

mExpClass(Well) Marker : public ::NamedMonitorable
{
public:

    typedef Strat::Level::ID	LevelID;
    typedef float		ZType;

			Marker(const char*,ZType d=0.f,Color c=Color());
			Marker(LevelID,ZType);
			~Marker();
			mDeclInstanceCreatedNotifierAccess(Marker);
			mDeclMonitorableAssignment(Marker);

    bool                operator >(const Marker&) const;

    virtual const OD::String& name() const;
    virtual BufferString getName() const;
    Color		color() const;

    mImplSimpleMonitoredGetSet(inline,dah,setDah,ZType,dah_,cDahChange());
    mImplSimpleMonitoredGetSet(inline,levelID,setLevelID,LevelID,levelid_,
							cLevelChange());
    mImplSimpleMonitoredGet(fallBackColor,Color,color_);
    mImplSimpleMonitoredSet(setColor,Color,color_,cColorChange());

    Strat::Level	getLevel() const;
    void		setNoLevelID();

    static const char*	sKeyDah();
    static ChangeType	cColorChange()		{ return 2; }
    static ChangeType	cLevelChange()		{ return 3; }
    static ChangeType	cDahChange()		{ return 4; } // like DahObj

    static const Marker& udf();
    bool		isUdf() const	{ return *this == udf(); }
    static Marker&	dummy();

protected:

    ZType		dah_;
    Color		color_;
    LevelID		levelid_;

			// Usable in locked state
    BufferString	gtName() const;
    Strat::Level	gtLevel() const;

};


/*!\brief Set of Markers */

mExpClass(Well) MarkerSet : public ::SharedObject
{
public:

    typedef Marker::LevelID	LevelID;
    typedef Marker::ZType	ZType;
    typedef TypeSet<Marker>::size_type size_type;
    typedef size_type		idx_type;
    mDefIntegerIDType(		MarkerID);


			MarkerSet();
			~MarkerSet();
			mDeclInstanceCreatedNotifierAccess(MarkerSet);
			mDeclMonitorableAssignment(MarkerSet);

    MarkerID		add(const Marker&);
    Marker		get(MarkerID) const;
    MarkerID		set(const Marker&);
    BufferString	getNameByID(MarkerID) const;
    void		setNameByID(MarkerID,const char*);
    Color		getColor(MarkerID) const;
    void		setColor(MarkerID,const Color&);
    ZType		getDah(MarkerID) const;
    void		setDah(MarkerID,ZType);
    void		removeSingle(MarkerID);

    Marker		getByIdx(idx_type) const;
    void		removeSingleByIdx(idx_type);
    ZType		getDahByIdx(idx_type) const;
    void		setDahByIdx(idx_type,ZType);

    ZType		getDahFromMarkerName(const char*) const;
    Marker		first() const;
    Marker		last() const;

    idx_type		indexOf(const char* nm) const;
    Marker		getByName(const char* nm) const;
    Marker		getByLvlID(LevelID id) const;
    MarkerID		markerIDFor(idx_type) const;
    MarkerID		markerIDFromName(const char*) const;
    idx_type		getIdxFor(MarkerID) const;
    bool		validIdx(idx_type) const;

    bool		isPresent(const char* n) const;
    bool		isEmpty() const;
    void		setEmpty();
    size_type		size() const;

    void		getNames(BufferStringSet&) const;
    void		getColors(TypeSet<Color>&) const;
    void		getMDs(TypeSet<ZType>&) const;

    void		fillWithAll(TaskRunner* tskr=0);
    bool		insertNew(const Well::Marker&);
    void		addSameWell(const MarkerSet&);
    void		mergeOtherWell(const MarkerSet&);
    void		append(const MarkerSet& ms)	{ mergeOtherWell(ms); }
    idx_type		getIdxAbove(ZType,const Well::Track* trck=0) const;
			//!< is trck provided, compares TVDs

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);


    static ChangeType	cMarkerAdded()		{ return 2; }
    static ChangeType	cMarkerRemoved()	{ return 3; }
    static ChangeType	cMarkerChanged()	{ return 4; }


protected:

    TypeSet<Marker>	markers_;
    TypeSet<MarkerID>	markerids_;
    mutable Threads::Atomic<idx_type> curmrkridnr_;

    Marker		gtByName(const char*) const; //without locks for own use
    Marker		gtByIndex(idx_type) const;
    Marker		gtByLvlID(LevelID) const;
    idx_type		idxOf(const char*) const;
    idx_type		gtIdxFor(MarkerID) const;
    idx_type		gtIdxForDah(ZType) const;
    Marker		gtByID(MarkerID) const;
    MarkerID		mrkrIDFor(idx_type) const;

    void		addCopy(const MarkerSet&,idx_type,ZType);
    void		alignOrderingWith(const MarkerSet&);
    void		moveBlock(idx_type,idx_type,const TypeSet<idx_type>&);
    bool		insrtNew(const Well::Marker&);
    void		insrtNewAfter(idx_type,const MarkerSet&);
    void		insrtAt(idx_type,const Marker&);
    void		insrtAfter(idx_type,const Marker&);
    void		rmoveSingle(idx_type);
    bool		isPrsnt(const char* n) const;
    size_type		gtSize() const;

    friend class	MarkerSetIter;
    friend class	MarkerSetIter4Edit;
};


/*!\brief const MarkerSet iterator.

  Pass empty (or null) name or invalid ID to start at first or end at last.

*/

mExpClass(Well) MarkerSetIter : public MonitorableIter4Read<MarkerSet::idx_type>
{
public:

    typedef MarkerSet::MarkerID	MarkerID;
    typedef MarkerSet::idx_type	idx_type;
    typedef MarkerSet::ZType	ZType;

			MarkerSetIter(const MarkerSet&,bool dorev=false);
			MarkerSetIter(const MarkerSet&,MarkerID,MarkerID);
			MarkerSetIter(const MarkerSet&,const char*,const char*);
			MarkerSetIter(const MarkerSetIter&);

    const MarkerSet&	markerSet() const;

    MarkerID		ID() const;

    const Marker&	get() const;
    ZType		getDah() const;
    BufferString	markerName() const;

    mDefNoAssignmentOper(MarkerSetIter)

};


/*!\brief edit-while-iterate for MarkerSet. Work on a local copy!

  Pass empty (or null) name for start at first marker or end at last.

*/

mExpClass(Well) MarkerSetIter4Edit
		    : public MonitorableIter4Write<MarkerSet::idx_type>
{
public:

    typedef MarkerSet::idx_type	idx_type;
    typedef MarkerSet::MarkerID	MarkerID;
    typedef MarkerSet::ZType	ZType;

			MarkerSetIter4Edit(MarkerSet&,bool dorev=false);
			MarkerSetIter4Edit(MarkerSet&,Interval<idx_type>);
			MarkerSetIter4Edit(MarkerSet&,const char*,const char*);
			MarkerSetIter4Edit(const MarkerSetIter4Edit&);

    MarkerSet&		markerSet();
    const MarkerSet&	markerSet() const;

    BufferString	markerName() const;
    MarkerID		ID() const;
    Marker&		get() const;
    ZType		getDah() const;
    void		setDah(ZType);
    void		setColor(const Color&);

    void		removeCurrent();
    void		insert(const Marker&);

    mDefNoAssignmentOper(MarkerSetIter4Edit)

};


/*!\brief Range of markers (typically describing zone of interest).
  As with iterators, pass null or invalid for start or stop. */

mExpClass(Well) MarkerRange
{
public:

    typedef MarkerSet::size_type	size_type;
    typedef MarkerSet::idx_type		idx_type;
    typedef MarkerSet::MarkerID		MarkerID;
    typedef MarkerSet::ZType		ZType;

			MarkerRange(const MarkerSet&,MarkerID,MarkerID);
			MarkerRange(const MarkerSet&,const char*,const char*);
    virtual		~MarkerRange();

    size_type		size() const;
    bool		isValid() const;

    bool		isIncluded(idx_type) const;
    bool		isIncluded(MarkerID) const;

    bool		isIncluded(const char*) const;
    bool		isIncluded(ZType) const;

    void		getNames(BufferStringSet&) const;
    MarkerSet*		getResultSet() const; //!< returns new set

    const MarkerSet&	markers() const		{ return markerset_; }

    void		setRangeForIDs(MarkerSet::MarkerID,MarkerSet::MarkerID);
    ZType		thickness() const;

protected:

    Interval<idx_type>	idxRange() const;
    const MarkerSet&	markerset_;
    MarkerSet::MarkerID	topid_;
    MarkerSet::MarkerID	botid_;

};


/*!\brief MarkerRange that can change its MarkerSet  */

mExpClass(Well) MarkerChgRange : public MarkerRange
{
public:

    typedef MarkerSet::ZType	ZType;

			MarkerChgRange( MarkerSet& ms, MarkerID tpid,
						       MarkerID btid )
			    : MarkerRange(ms,tpid,btid)	{}

			MarkerChgRange( MarkerSet& ms, const char* topmrkr,
						       const char* botmrkr )
			    : MarkerRange(ms,topmrkr,botmrkr) {}


    void		setThickness(ZType);
    void		remove();

    inline MarkerSet&	getMarkers()
			{ return const_cast<MarkerSet&>(markerset_); }

};


} // namespace Well
