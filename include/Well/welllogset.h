#ifndef welllogset_h
#define welllogset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welllogset.h,v 1.7 2004-05-09 15:17:12 bert Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "position.h"
#include "ranges.h"

namespace Well
{

class Log;

class LogSet
{
public:

			LogSet()		{ init(); }
    virtual		~LogSet();

    int			size() const		{ return logs.size(); }
    Log&		getLog( int idx )	{ return *logs[idx]; }
    const Log&		getLog( int idx ) const	{ return *logs[idx]; }
    int			indexOf(const char*) const;

    Interval<float>	dahInterval() const	{ return dahintv; }
    						//!< not def if start == undef
    void		updateDahIntvs();
    						//!< if logs changed

    void		add(Log*);		//!< becomes mine
    Log*		remove(int);		//!< becomes yours

protected:

    ObjectSet<Log>	logs;
    Interval<float>	dahintv;

    void		init()
    			{ dahintv.start = dahintv.stop = mUndefValue; }

    void		updateDahIntv(const Well::Log&);

};


}; // namespace Well

#endif
