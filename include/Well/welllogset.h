#ifndef welllogset_h
#define welllogset_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welllogset.h,v 1.1 2003-08-15 11:12:15 bert Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "position.h"
#include "ranges.h"

namespace Well
{

class LogSet
{
public:

			LogSet()		{ init(); }
    virtual		~LogSet();

    int			nrLogs() const		{ return logs.size(); }
    Log&		getLog( int idx )	{ return *logs[idx]; }
    const Log&		getLog( int idx ) const	{ return *logs[idx]; }

    Interval<float>	dahInterval() const	{ return dahintv; }
    						//!< not def if start == undef

    void		add(Log*);		//!< becomes mine
    Log*		remove(int);		//!< becomes yours

protected:

    ObjectSet<Log>	logs;
    Interval<float>	dahintv;

    void		init()
    			{	dahintv.start = mUndefValue;
				dahintv.stop = -mUndefValue; }

};


}; // namespace Well

#endif
