#ifndef welllogset_h
#define welllogset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellmod.h"
#include "position.h"
#include "ranges.h"
#include "propertyref.h"
class BufferStringSet;


namespace Well
{

class Log;

/*!
\brief Log set
*/

mExpClass(Well) LogSet
{
public:

			LogSet()		{ init(); }
    virtual		~LogSet()		{ setEmpty(); }

    void		getNames(BufferStringSet&) const;

    int			size() const		{ return logs_.size(); }
    Log&		getLog( int idx )	{ return *logs_[idx]; }
    const Log&		getLog( int idx ) const	{ return *logs_[idx]; }
    int			indexOf(const char*) const;
    const Log*		getLog( const char* nm ) const	{ return gtLog(nm); }
    Log*		getLog( const char* nm )	{ return gtLog(nm); }

    Interval<float>	dahInterval() const	{ return dahintv_; }
						//!< not def if start == undef
    void		updateDahIntvs();
						//!< if logs changed
    void		removeTopBottomUdfs();

    void		add(Log*);		//!< becomes mine
    Log*		remove(int);		//!< becomes yours
    void		swap(int idx0,int idx1)	{ logs_.swap( idx0, idx1 ); }
    bool		validIdx(int idx) const	{ return logs_.validIdx(idx); }

    bool		isEmpty() const		{ return size() == 0; }
    void		setEmpty();

    TypeSet<int>	getSuitable(PropertyRef::StdType,
				    const PropertyRef* altpr=0,
				    BoolTypeSet* isalt=0) const;

protected:

    ObjectSet<Log>	logs_;
    Interval<float>	dahintv_;

    void		init()
			{ dahintv_.start = mSetUdf(dahintv_.stop); }

    void		updateDahIntv(const Well::Log&);

    Log*		gtLog( const char* nm ) const
			{ const int idx = indexOf(nm);
			    return idx < 0 ? 0 : const_cast<Log*>(logs_[idx]); }

};

} // namespace Well

#endif
