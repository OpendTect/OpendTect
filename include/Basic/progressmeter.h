#ifndef progressmeter_h
#define progressmeter_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl / Bert Bril
 Date:          07-10-1999
 RCS:           $Id: progressmeter.h,v 1.5 2001-05-31 12:55:07 windev Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include <limits.h>
#include <iosfwd>


/*!\brief Textual progress indicator for batch programs. */

class ProgressMeter
{
public:
			ProgressMeter(ostream&,unsigned long dist=1,
				      unsigned short rowlen=50,
				      bool finishondestruct=true);
    virtual		~ProgressMeter();

    unsigned long	update(unsigned long auxdisp=ULONG_MAX);
    unsigned long	operator++();
    void		reset();
    void		finish();

protected:

    ostream&		strm;
    unsigned short	rowlen;
    short		idist;
    unsigned long	dist;
    unsigned long       progress;
    unsigned long	zeropoint;
    unsigned long	auxnr;
    int 		oldtime; 
    bool		destrfin;
    bool		inited;
    bool		finished;

    void		annotate(bool);

}; 


#endif
