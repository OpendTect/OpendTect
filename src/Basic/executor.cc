/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 14-6-1996
 * FUNCTION : functions
-*/

static const char* rcsID = "$Id: executor.cc,v 1.1 2000-03-02 15:28:55 bert Exp $";

#include "executor.h"
#include "timefun.h"
#include <iostream.h>


bool Executor::execute( ostream* strm, bool isfirst, bool islast )
{
    if ( !strm )
    {
	int rv = 1;
	while ( rv )
	{
	    rv = nextStep();
	    if ( rv < 0 )
	    {
		cerr << (message() ? message() : "") << endl;
		return NO;
	    }
	}
	return YES;
    }

    ostream& stream = *strm;
    if ( isfirst )
	stream << GetProjectVersionName() << endl << endl;

    stream << "Process: '" << name() << "'" << endl;
    stream << "Started: " << Time_getLocalString() << endl << endl;

    UserIDString curmsg, prevmsg;
    prevmsg = message();

    if ( strm )
	stream << '\t' << prevmsg << endl;

    int go_on = YES;
    int newmsg = YES;
    int needendl = NO;
    int nrdone = 0;
    int nrdonedone = 0;
    int rv;
    while ( go_on )
    {
	rv = nextStep();
	curmsg = message();
	int newnrdone = nrDone();
	go_on = NO;
	switch( rv )
	{
	case -1:
	    stream << "Error: " << curmsg << endl;
	break;
	case  0:
	    stream << "\nFinished: " << Time_getLocalString() << endl;
	break;
	default:
	    go_on = YES;
	    if ( curmsg != prevmsg )
	    {
		if ( needendl ) stream << endl;
		needendl = NO;
		stream << '\t' << curmsg << endl;
		newmsg = YES;
	    }
	    else if ( newmsg && newnrdone )
	    {
		newmsg = NO;
		if ( !nrdone )
		    stream << '\t' << nrDoneText() << ":\n\t\t";
		stream << newnrdone;
		nrdonedone = 1;
		needendl = YES;
	    }
	    else if ( newnrdone && newnrdone != nrdone )
	    {
		nrdonedone++;
		needendl = nrdonedone%8 ? YES : NO;
		stream << (nrdonedone%8 ? " " : "\n\t\t");
		stream << newnrdone;
		stream.flush();
	    }
	break;
	}
	nrdone = newnrdone;
	prevmsg = curmsg;
    }

    if ( islast )
	stream << endl << endl << "End of processing" << endl;
    return rv < 0 ? NO : YES;
}
