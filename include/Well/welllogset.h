#ifndef welllogset_h
#define welllogset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welllogset.h,v 1.16 2012-08-03 13:00:45 cvskris Exp $
________________________________________________________________________


-*/

#include "wellmod.h"
#include "sets.h"
#include "position.h"
#include "ranges.h"

namespace Well
{

class Log;

mClass(Well) LogSet
{
public:

			LogSet()		{ init(); }
    virtual		~LogSet()		{ empty(); }

    int			size() const		{ return logs.size(); }
    Log&		getLog( int idx )	{ return *logs[idx]; }
    const Log&		getLog( int idx ) const	{ return *logs[idx]; }
    int			indexOf(const char*) const;
    const Log*		getLog( const char* nm ) const	{ return gtLog(nm); }
    Log*		getLog( const char* nm )	{ return gtLog(nm); }

    Interval<float>	dahInterval() const	{ return dahintv; }
    						//!< not def if start == undef
    void		updateDahIntvs();
    						//!< if logs changed
    void		removeTopBottomUdfs();

    void		add(Log*);		//!< becomes mine
    Log*		remove(int);		//!< becomes yours
    void		swap(int idx0,int idx1)	{ logs.swap( idx0, idx1 ); }
    bool		validIdx(int idx) const	{ return logs.validIdx(idx); }

    bool		isEmpty() const		{ return size() == 0; }
    void		empty();

protected:

    ObjectSet<Log>	logs;
    Interval<float>	dahintv;

    void		init()
    			{ dahintv.start = mSetUdf(dahintv.stop); }

    void		updateDahIntv(const Well::Log&);

    Log*		gtLog( const char* nm ) const
			{ const int idx = indexOf(nm);
			    return idx < 0 ? 0 : const_cast<Log*>(logs[idx]); }

};


}; // namespace Well

#endif

