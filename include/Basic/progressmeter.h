#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl / Bert
 Date:          07-10-1999
________________________________________________________________________

-*/

#include "basicmod.h"
#include "uistring.h"


/*!\brief is an interface where processes can report their progress. */

mExpClass(Basic) ProgressMeter
{
public:

    virtual		~ProgressMeter()		{}

    virtual void	setStarted()			{}
    virtual void	setFinished()			{}

    virtual od_int64	nrDone() const			{ return -1; }
    virtual void	setName(const char*)		{}
    virtual void	setTotalNr(od_int64)		{}
    virtual void	setNrDone(od_int64)		{}
    virtual void	setNrDoneText(const uiString&)	{}
    virtual void	setMessage(const uiString&)	{}
    virtual void	printMessage(const uiString&)	{}

    virtual void	operator++()			= 0;

			/*!Force to skip progress info. */
    virtual void	skipProgress(bool yn)		{}
};
