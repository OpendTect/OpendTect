#ifndef strmdata_H
#define strmdata_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		3-4-1996
 Contents:	Data on any stream
 RCS:		$Id: strmdata.h,v 1.6 2002-12-03 14:51:39 bert Exp $
________________________________________________________________________

-*/
 
#include "gendefs.h"
#include <stdio.h>
#include <iosfwd>


/*!\brief holds data to use and close an iostream.

Usualyy created by StreamProvider.
Need to find out what to do with the pipe in windows.

*/

class StreamData
{
public:
		StreamData() : ispipe(false)	{ init(); }

    void	close();
    bool	usable() const;

    istream*	istrm;
    ostream*	ostrm;

    FILE*	fp;
    bool	ispipe;

private:

    void	init();

};


#endif
