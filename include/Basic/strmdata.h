#ifndef strmdata_H
#define strmdata_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		3-4-1996
 Contents:	Data on any stream
 RCS:		$Id: strmdata.h,v 1.4 2001-05-31 12:55:10 windev Exp $
________________________________________________________________________

-*/
 
#include <stdio.h>
#include <iosfwd>

class stdiobuf;


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
    stdiobuf*	sb;
    bool	ispipe;

private:

    void	init();

};


#endif
