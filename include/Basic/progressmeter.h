#ifndef progressmeter_h
#define progressmeter_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl / Bert Bril
 Date:          07-10-1999
 RCS:           $Id: progressmeter.h,v 1.10 2004-04-27 15:51:15 bert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include <limits.h>
#include <iosfwd>


/*!\brief Textual progress indicator for batch programs. */

class ProgressMeter
{
public:
			ProgressMeter(std::ostream&,unsigned long dist=1,
				      unsigned short rowlen=50,
				      bool finishondestruct=true);
    virtual		~ProgressMeter();

    unsigned long	update(unsigned long auxdisp=ULONG_MAX);
    unsigned long	operator++();
    void		reset();
    void		resetDist();
    void		finish();

    unsigned long	nrDone() const		{ return progress; }

protected:

    std::ostream&	strm;
    unsigned short	rowlen;
    unsigned char	idist;
    unsigned long	dist;
    unsigned long       progress;
    unsigned long	lastannotatedprogress;
    unsigned long	auxnr;
    int 		oldtime; 
    int 		nrdotsonline; 
    bool		destrfin;
    bool		inited;
    bool		finished;

    void		annotate(bool);

}; 


#endif
