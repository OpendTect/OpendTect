#ifndef progressmeter_h
#define progressmeter_h

/*@+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: progressmeter.h,v 1.3 2000-08-04 14:35:17 bert Exp $
________________________________________________________________________

@$*/

#include "gendefs.h"
#include <limits.h>
class ostream;


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
