#ifndef strmdata_H
#define strmdata_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		3-4-1996
 Contents:	Data on any stream
 RCS:		$Id: strmdata.h,v 1.1 2000-03-02 15:24:34 bert Exp $
________________________________________________________________________

@$*/
 
 
#include <stdio.h>
class istream;
class ostream;
class stdiobuf;


/*$@ StreamData
 Data on stream and underlying stuff.
@$*/

class StreamData
{
public:
		StreamData() : ispipe(0)	{ init(); }

    void	close();
    int		usable() const;

    istream*	istrm;
    ostream*	ostrm;

    FILE*	fp;
    stdiobuf*	sb;
    int		ispipe;

private:

    void	init();

};


/*$-*/
#endif
