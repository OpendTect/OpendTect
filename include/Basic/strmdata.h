#ifndef strmdata_H
#define strmdata_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		3-4-1996
 Contents:	Data on any stream
 RCS:		$Id: strmdata.h,v 1.3 2001-02-13 17:15:46 bert Exp $
________________________________________________________________________

-*/
 
#include <stdio.h>
class istream;
class ostream;
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
