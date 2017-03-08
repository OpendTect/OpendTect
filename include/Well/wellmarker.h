#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
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

    mImplSimpleMonitoredGetSet(inline,dah,setDah,ZType,dah_,cDahChange());
    mImplSimpleMonitoredGetSet(inline,levelID,setLevelID,LevelID,levelid_,
							cLevelChange());
    Color		color() const;
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

    float		dah_;
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
    typedef TypeSet<Marker>::size_type    size_type;
    typedef size_type		IdxType;
    mDefIntegerIDType(IdxType,	 MarkerID);


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
    float		getDah(MarkerID) const;
    void		setDah(MarkerID,float);
    void		removeSingle(MarkerID);

    Marker		getByIdx(IdxType) const;
    void		removeSingleByIdx(IdxType);
    float		getDahByIdx(IdxType) const;
    void		setDahByIdx(IdxType,float);

    float		getDahFromMarkerName(const char*) const;
    Marker		first() const;
    Marker		last() const;

    IdxType		indexOf(const char* nm) const;
    Marker		getByName(const char* nm) const;
    Marker		getByLvlID(LevelID id) const;
    MarkerID		markerIDFor(IdxType) const;
    MarkerID		markerIDFromName(const char*) const;
    IdxType		getIdxFor(MarkerID) const;
    bool		validIdx(IdxType) const;

    bool		isPresent(const char* n) const;
    bool		isEmpty() const;
    void		setEmpty();
    size_type		size() const;

    void		getNames(BufferStringSet&) const;
    void		getColors(TypeSet<Color>&) const;

    void		fillWithAll(TaskRunner* tskr=0);
    bool		insertNew(const Well::Marker&);
    void		addSameWell(const MarkerSet&);
    void		mergeOtherWell(const MarkerSet&);
    void		append(const MarkerSet& ms)
							{ mergeOtherWell(ms); }
    IdxType		getIdxAbove(float z,const Well::Track* trck=0) const;
			//!< is trck provided, compares TVDs

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);


    static ChangeType	cMarkerAdded()		{ return 2; }
    static ChangeType	cMarkerRemoved()	{ return 3; }
    static ChangeType	cMarkerChanged()	{ return 4; }


protected:

    TypeSet<Marker>	markers_;
    TypeSet<MarkerID>	markerids_;
    mutable Threads::Atomic<IdxType> curmrkridnr_;

    Marker		gtByName(const char*) const; //without locks for own use
    Marker		gtByIndex(IdxType) const;
    Marker		gtByLvlID(LevelID) const;
    IdxType		idxOf(const char*) const;
    IdxType		gtIdxFor(MarkerID) const;
    IdxType		gtIdxForDah(float) const;
    Marker		gtByID(MarkerID) const;
    MarkerID		mrkrIDFor(IdxType) const;

    void		addCopy(const MarkerSet&,IdxType,float);
    void		alignOrderingWith(const MarkerSet&);
    void		moveBlock(IdxType,IdxType,const TypeSet<IdxType>&);
    bool		insrtNew(const Well::Marker&);
    void		insrtNewAfter(IdxType,const MarkerSet&);
    void		insrtAt(IdxType,const Marker&);
    void		insrtAfter(IdxType,const Marker&);
    void		rmoveSingle(IdxType);
    bool		isPrsnt(const char* n) const;
    size_type		gtSize() const;

    friend class	MarkerSetIter;
    friend class	MarkerSetIter4Edit;
};


/*!\brief const MarkerSet iterator.

  Pass empty (or null) name or invalid ID to start at first or end at last.

*/

mExpClass(Well) MarkerSetIter : public MonitorableIter4Read<MarkerSet::IdxType>
{
public:

    typedef MarkerSet::MarkerID	MarkerID;

			MarkerSetIter(const MarkerSet&,bool dorev=false);
			MarkerSetIter(const MarkerSet&,MarkerID,MarkerID);
			MarkerSetIter(const MarkerSet&,const char*,const char*);
			MarkerSetIter(const MarkerSetIter&);

    const MarkerSet&	markerSet() const;

    MarkerID		ID() const;

    const Marker&	get() const;
    float		getDah() const;
    BufferString	markerName() const;

    mDefNoAssignmentOper(MarkerSetIter)

};


/*!\brief edit-while-iterate for MarkerSet. Work on a local copy!

  Pass empty (or null) name for start at first marker or end at last.

*/

mExpClass(Well) MarkerSetIter4Edit
		    : public MonitorableIter4Write<MarkerSet::IdxType>
{
public:

    typedef MarkerSet::IdxType	IdxType;
    typedef MarkerSet::MarkerID	MarkerID;

			MarkerSetIter4Edit(MarkerSet&,bool dorev=false);
			MarkerSetIter4Edit(MarkerSet&,Interval<IdxType>);
			MarkerSetIter4Edit(MarkerSet&,const char*,const char*);
			MarkerSetIter4Edit(const MarkerSetIter4Edit&);

    MarkerSet&		markerSet();
    const MarkerSet&	markerSet() const;

    BufferString	markerName() const;
    MarkerID		ID() const;
    Marker&		get() const;
    float		getDah() const;
    void		setDah(float);
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
    typedef MarkerSet::IdxType		IdxType;
    typedef MarkerSet::MarkerID		MarkerID;

			MarkerRange(const MarkerSet&,MarkerID,MarkerID);
			MarkerRange(const MarkerSet&,const char*,const char*);
    virtual		~MarkerRange();

    size_type		size() const;
    bool		isValid() const;

    bool		isIncluded(IdxType) const;
    bool		isIncluded(MarkerID) const;

    bool		isIncluded(const char*) const;
    bool		isIncluded(float z) const;

    void		getNames(BufferStringSet&) const;
    MarkerSet*		getResultSet() const; //!< returns new set

    const MarkerSet&	markers() const		{ return markerset_; }

    void		setRangeForIDs(MarkerSet::MarkerID,MarkerSet::MarkerID);
    float		thickness() const;

protected:

    Interval<IdxType>	idxRange() const;
    const MarkerSet&	markerset_;
    MarkerSet::MarkerID	topid_;
    MarkerSet::MarkerID	botid_;

};


/*!\brief MarkerRange that can change its MarkerSet  */

mExpClass(Well) MarkerChgRange : public MarkerRange
{
public:

			MarkerChgRange( MarkerSet& ms, MarkerID tpid,
						       MarkerID btid )
			    : MarkerRange(ms,tpid,btid)	{}

			MarkerChgRange( MarkerSet& ms, const char* topmrkr,
						       const char* botmrkr )
			    : MarkerRange(ms,topmrkr,botmrkr) {}


    void		setThickness(float);
    void		remove();

    inline MarkerSet&	getMarkers()
			{ return const_cast<MarkerSet&>(markerset_); }

};


} // namespace Well
