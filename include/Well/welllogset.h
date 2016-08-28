#ifndef welllogset_h
#define welllogset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "welllog.h"
#include "propertyref.h"
class BufferStringSet;


namespace Well
{

/*!\brief Set of Well::Log's. */

mExpClass(Well) LogSet : public NamedMonitorable
{
public:

    typedef ObjectSet<Log>::size_type	size_type;
    typedef size_type			IdxType;
    typedef IntegerID<IdxType>		LogID;
    typedef Well::DahObj::ZType		ZType;
    typedef RefMan<Log>			LogRefMan;
    typedef ConstRefMan<Log>		CLogRefMan;

			LogSet();
			~LogSet();
			mDeclMonitorableAssignment(LogSet);
			mDeclInstanceCreatedNotifierAccess(LogSet);

    LogRefMan		getLog(LogID);
    CLogRefMan		getLog(LogID) const;
    LogRefMan		getLogByName(const char*);
    CLogRefMan		getLogByName(const char*) const;
    LogRefMan		getLogByIdx(IdxType);
    CLogRefMan		getLogByIdx(IdxType) const;
    LogRefMan		firstLog();
    CLogRefMan		firstLog() const;

    size_type		size() const;
    int			indexOf(LogID) const;
    int			indexOf(const char*) const;
    bool		validIdx(IdxType) const;
    bool		isEmpty() const		{ return size() == 0; }
    void		setEmpty();

    LogID		add(Log*);
    LogRefMan		remove(LogID);
    LogRefMan		removeByName(const char*);

    bool		isPresent(const char*) const;
    void		getNames(BufferStringSet&) const;
    mImplSimpleMonitoredGet(dahInterval,Interval<ZType>,dahintv_)
						//!< not def if start == undef

    void		removeTopBottomUdfs();	//!< for all logs

    TypeSet<LogID>	getSuitable(PropertyRef::StdType,
				    const PropertyRef* altpr=0,
				    BoolTypeSet* isalt=0) const;

    static ChangeType	cLogAdd()	{ return 2; }
    static ChangeType	cLogRemove()	{ return 3; }
    static ChangeType	cOrderChange()	{ return 4; }

protected:

    ObjectSet<Log>	logs_;
    TypeSet<LogID>	logids_;
    Interval<ZType>	dahintv_;
    mutable Threads::Atomic<IdxType> curlogidnr_;

    IdxType		gtIdx(LogID) const;
    Log*		gtLog(LogID) const;
    LogID		gtID(const Log*) const;
    IdxType		gtIdxByName(const char*) const;
    Log*		gtLogByName(const char*) const;
    Log*		gtLogByIdx(IdxType) const;
    void		updateDahIntv(const Well::Log&);
    void		recalcDahIntv();
    Log*		doRemove(IdxType);
    void		doSetEmpty();

    friend class	LogSetIter;

public:

    bool		swap(IdxType,IdxType);

};


mExpClass(Well) LogSetIter : public MonitorableIter<Log::IdxType>
{
public:

    typedef LogSet::LogID   LogID;

			LogSetIter(const LogSet&,bool start_at_end=false);
			LogSetIter(const LogSetIter&);
    const LogSet&	logSet() const;
    size_type		size() const;

    LogID		ID() const;
    const Log&		log() const;

};


} // namespace Well

#endif
