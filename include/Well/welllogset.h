#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellmod.h"

#include "mnemonics.h"
#include "position.h"
#include "ranges.h"

class BufferStringSet;
class PropertyRef;

namespace Well
{

class Log;

/*!
\brief Log set
*/

mExpClass(Well) LogSet : public CallBacker
{
public:

			LogSet();
    virtual		~LogSet();

    void		getNames(BufferStringSet&, bool onlyloaded=false) const;

    int			size() const		{ return logs_.size(); }
    Log&		getLog( int idx )	{ return *logs_[idx]; }
    const Log&		getLog( int idx ) const	{ return *logs_[idx]; }
    int			indexOf(const char*) const;
    bool		isLoaded(const char*) const;
    bool		isPresent(const char*) const;
    const Log*		getLog( const char* nm ) const	{ return gtLog(nm); }
    Log*		getLog( const char* nm )	{ return gtLog(nm); }

    Interval<float>	dahInterval() const	{ return dahintv_; }
						//!< not def if start == undef
    void		updateDahIntvs();
						//!< if logs changed
    void		removeTopBottomUdfs();

    void		add(Log*);		//!< becomes mine
    void		add(const LogSet&);	//!< copies all logs
    Log*		remove(int);		//!< becomes yours
    void		swap(int idx0,int idx1)	{ logs_.swap( idx0, idx1 ); }
    bool		validIdx(int idx) const	{ return logs_.validIdx(idx); }

    bool		isEmpty() const		{ return size() == 0; }
    void		setEmpty();

    TypeSet<int>	getSuitable(Mnemonic::StdType,
				    const PropertyRef* altpr=nullptr,
				    BoolTypeSet* isalt=nullptr) const;

    Notifier<LogSet>	logAdded;
    Notifier<LogSet>	logRemoved;

protected:

    ObjectSet<Log>	logs_;
    Interval<float>	dahintv_;

    void		init()
			{ dahintv_.start = mSetUdf(dahintv_.stop); }

    void		updateDahIntv(const Well::Log&);

    Log*		gtLog( const char* nm ) const
			{ const int idx = indexOf(nm);
			    return idx < 0 ? 0 : const_cast<Log*>(logs_[idx]); }

private:
			LogSet(const LogSet&)		= delete;
    LogSet&		operator= (const LogSet&)	= delete;

};

} // namespace Well

