#ifndef welllogset_h
#define welllogset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welllogset.h,v 1.11 2009-02-27 14:15:52 cvsbert Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "position.h"
#include "ranges.h"

namespace Well
{

class Log;

mClass LogSet
{
public:

			LogSet()		{ init(); }
    virtual		~LogSet();

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

    void		add(Log*);		//!< becomes mine
    Log*		remove(int);		//!< becomes yours

    bool		isEmpty() const		{ return size() == 0; }

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
