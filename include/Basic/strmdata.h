#ifndef strmdata_H
#define strmdata_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		3-4-1996
 Contents:	Data on any stream
 RCS:		$Id: strmdata.h,v 1.5 2001-06-02 14:28:28 windev Exp $
________________________________________________________________________

-*/
 
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
