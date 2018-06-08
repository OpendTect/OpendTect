#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2004
________________________________________________________________________


-*/

#include "wellcommon.h"
#include "namedmonitorable.h"
#include "monitoriter.h"
#include "ranges.h"
#include "sets.h"
#include "integerid.h"


namespace Well
{

/*!\brief object with a geometry defined by depth along hole.

  Well tracks define a new coord system that determines the position of
  any point in the well by a single coordinate: the depth-along-hole (aka MD).

  Every DahObject has at least one value that determines the object's raison
  d'etre. Also, every 'defined' point along the track has its own ID which will
  never change during this session: the PointID.

*/

mExpClass(Well) DahObj : public ::NamedMonitorable
{
public:

    typedef float		ZType;
    typedef TypeSet<ZType>	ZSetType;
    typedef Interval<ZType>	ZIntvType;
    typedef ZSetType::size_type	size_type;
    typedef size_type		idx_type;
    typedef IntegerID<idx_type>	PointID;
    typedef float		ValueType;
    typedef TypeSet<ValueType>	ValueSetType;
    typedef Interval<ValueType>	ValueIntvType;

			DahObj(const char* nm=0);
    virtual		~DahObj();
			mDeclInstanceCreatedNotifierAccess(DahObj);
			mDeclAbstractMonitorableAssignment(DahObj);

    size_type		size() const;
    inline bool		isEmpty() const			{ return size() < 1; }
    ZType		dah(PointID) const;
    ZType		firstDah() const;
    ZType		lastDah() const;
    ValueType		value(PointID) const;
    ValueType		valueAt(ZType,bool no_undefs=false) const;
    ValueType		firstValue(bool no_undefs=false) const;
    ValueType		lastValue(bool no_undefs=false) const;
    ZIntvType		dahRange() const;
    ZType		dahStep(bool min_else_average) const;
    void		getDahValues(ZSetType&) const;
    virtual void	getData(ZSetType&,ValueSetType&) const	= 0;
    virtual bool	valsAreCodes() const		{ return false; }
    PointID		firstID() const;
    PointID		lastID() const;
    PointID		nextID(PointID) const;
    PointID		prevID(PointID) const;
    PointID		nearestID(ZType) const ;

    void		setEmpty();
    void		setDah(PointID,ZType);
    bool		setValue(PointID,ValueType);
    PointID		setValueAt(ZType,ValueType);
    void		remove(PointID);
    void		deInterpolate();	//!< Remove unnecessary points
    void		shiftDahFrom(PointID,ZType);
    void		convertZ(bool tofeet);

			// Use MonitorLock when iterating
    PointID		pointIDFor(idx_type) const;
    idx_type		indexOf(PointID) const;
    idx_type		indexOf(ZType) const;
    ZType		dahByIdx(idx_type) const;
    ValueType		valueByIdx(idx_type) const;
    void		removeByIdx(idx_type);

    static ChangeType	cParsChange()		{ return 2; }
    static ChangeType	cPointAdd()		{ return 3; }
    static ChangeType	cDahChange()		{ return 4; }
    static ChangeType	cValueChange()		{ return 5; }
    static ChangeType	cPointRemove()		{ return 6; }
    static ZType	dahEps()		{ return 1e-5f; }

protected:

    ZSetType		dahs_;
    TypeSet<PointID>	ptids_;
    mutable Threads::Atomic<idx_type> curptidnr_;

			// fns for for already locked state

    virtual bool	doSet(idx_type,ValueType)		= 0;
    virtual PointID	doInsAtDah(ZType,ValueType)		= 0;
    virtual ValueType	gtVal(idx_type) const			= 0;
    virtual void	removeAux(idx_type)			= 0;
    virtual void	eraseAux()				= 0;

    void		doRemove(idx_type);
    void		doSetEmpty();
    PointID		doIns(ZType dh,ValueType val,ValueSetType&,
				bool ascendingvalsonly);
    bool		doSetData(const ZSetType&,const ValueSetType&,
				  ValueSetType&);

    size_type		gtSize() const		{ return dahs_.size(); }
    idx_type		gtIsEmpty() const	{ return dahs_.isEmpty(); }
    PointID		gtNeighbourID(PointID,bool) const;
    PointID		gtNewPointID() const;
    idx_type		gtIndexOf(ZType) const;
    idx_type		gtIdx(PointID) const;
    ZType		gtDah(idx_type) const;
    ValueType		gtValueAt(ZType,bool noudfs) const;
    static bool		areEqualDahs(ZType,ZType);

    friend class	DahObjIter;

};


mExpClass(Well) DahObjIter : public MonitorableIter4Read<DahObj::idx_type>
{
public:

    typedef DahObj::PointID	PointID;
    typedef DahObj::ZType	ZType;
    typedef DahObj::ValueType	ValueType;

			DahObjIter(const DahObj&,bool start_at_end=false);
			DahObjIter(const DahObjIter&);

    const DahObj&	dahObj() const;
    PointID		ID() const;

    ZType		dah() const;
    ValueType		value() const;

};


}; // namespace Well
