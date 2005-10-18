/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 14-6-1996
-*/

static const char* rcsID = "$Id: executor.cc,v 1.19 2005-10-18 16:28:56 cvsbert Exp $";

#include "executor.h"
#include "timefun.h"
#include "oddirs.h"
#include "errh.h"
#include <iostream>

const int Executor::ErrorOccurred	= -1;
const int Executor::Finished		= 0;
const int Executor::MoreToDo		= 1;
const int Executor::WarningAvailable	= 2;


bool Executor::execute( std::ostream* strm, bool isfirst, bool islast,
		        int delaybetwnsteps )
{
    if ( !strm )
    {
	int rv = MoreToDo;
	while ( rv )
	{
	    rv = doStep();
	    if ( rv < 0 )
	    {
		const char* msg = message();
		if ( msg && *msg ) ErrMsg( msg );
		return false;
	    }
	    if ( delaybetwnsteps )
		Time_sleep( delaybetwnsteps*0.001 );
	}
	return true;
    }

    std::ostream& stream = *strm;
    if ( isfirst )
	stream << GetProjectVersionName() << "\n\n";

    stream << "Process: '" << name() << "'\n";
    stream << "Started: " << Time_getFullDateString() << "\n\n";

    BufferString curmsg, prevmsg;
    prevmsg = message();

    if ( strm )
	stream << '\t' << prevmsg << '\n';

    bool go_on = true;
    bool newmsg = true;
    bool needendl = false;
    int nrdone = 0;
    int nrdonedone = 0;
    int rv;
    while ( go_on )
    {
	rv = doStep();
	curmsg = message();
	int newnrdone = nrDone();
	go_on = false;
	switch( rv )
	{
	case ErrorOccurred:
	    stream << "Error: " << curmsg << std::endl;
	break;
	case Finished:
	    stream << "\nFinished: " << Time_getFullDateString() << std::endl;
	break;
	default:
	    go_on = true;
	    if ( curmsg != prevmsg )
	    {
		if ( needendl ) stream << std::endl;
		needendl = false;
		stream << '\t' << curmsg << std::endl;
		newmsg = true;
	    }
	    else if ( newmsg && newnrdone )
	    {
		newmsg = false;
		if ( !nrdone )
		    stream << '\t' << nrDoneText() << ":\n\t\t";
		stream << newnrdone;
		nrdonedone = 1;
		needendl = true;
	    }
	    else if ( newnrdone && newnrdone != nrdone )
	    {
		nrdonedone++;
		needendl = nrdonedone%8 ? true : false;
		stream << (nrdonedone%8 ? " " : "\n\t\t");
		stream << newnrdone;
		stream.flush();
	    }
	break;
	}
	nrdone = newnrdone;
	prevmsg = curmsg;
	if ( delaybetwnsteps )
	    Time_sleep( delaybetwnsteps*0.001 );
    }

    if ( islast )
	stream << "\n\nEnd of processing" << std::endl;
    return rv < 0 ? false : true;
}


int Executor::doStep()
{
    prestep.trigger();
    int res = nextStep();
    if ( res > 0 )
	poststep.trigger();
    return res;
}


ExecutorGroup::ExecutorGroup( const char* nm, bool p )
	: Executor( nm )
	, executors( *new ObjectSet<Executor> )
	, nrdone( 0 )
	, currentexec( 0 )
    	, paralell( p )
{
}


ExecutorGroup::~ExecutorGroup()
{
    deepErase( executors );
    delete &executors;
}


bool ExecutorGroup::similarLeft() const
{
    for ( int idx=currentexec+1; idx<executors.size(); idx++ )
    {
	if ( strcmp(executors[idx]->nrDoneText(),
		    executors[idx-1]->nrDoneText())
	  || strcmp(executors[idx]->message(),
		    executors[idx-1]->message()) )
	    return false;
    }
    return true;
}


void ExecutorGroup::add( Executor* n )
{
    executors += n;
    executorres += 1;
}


int ExecutorGroup::nextStep()
{
    const int nrexecs = executors.size();
    if ( !nrexecs ) return Finished;

    int res = executorres[currentexec] = executors[currentexec]->doStep();
    if ( res == ErrorOccurred )
	return ErrorOccurred;
    else if ( paralell || res==Finished )
	res = goToNextExecutor() ? MoreToDo : Finished;

    nrdone++;
    return res;
}

bool ExecutorGroup::goToNextExecutor()
{
    const int nrexecs = executors.size();
    if ( !nrexecs ) return false;

    for ( int idx=0; idx<nrexecs; idx++ )
    {
	currentexec++;
	if ( currentexec==nrexecs )
	    currentexec = 0;

	if ( executorres[currentexec]>Finished )
	    return true;
    }

    return false;
}


const char* ExecutorGroup::message() const
{
    return executors.size() ? executors[currentexec]->message()
			    : Executor::message();
}


int ExecutorGroup::totalNr() const
{
    const int nrexecs = executors.size();
    if ( !nrexecs ) return -1;

    if ( !paralell && !similarLeft() )
	return executors[currentexec]->totalNr();

    int totnr = 0;
    for ( int idx=0; idx<nrexecs; idx++ )
    {
	int nr = executors[idx]->totalNr();
	if ( nr < 0 ) return -1;

	totnr += nr;
    }

    return totnr;
}


int ExecutorGroup::nrDone() const
{
    const int nrexecs = executors.size();
    if ( nrexecs < 1 )
	return -1;

    if ( paralell || similarLeft() )
    {
	int totnr = 0;
	for ( int idx=0; idx<nrexecs; idx++ )
	    totnr += executors[idx]->nrDone();
	return totnr;
    }

    return executors[currentexec]->nrDone();
}


const char* ExecutorGroup::nrDoneText() const
{
    const char* txt = (const char*)nrdonetext;
    if ( *txt ) return txt;

    if ( !executors.size() )
	return Executor::nrDoneText();

    return executors[currentexec]->nrDoneText();
}
