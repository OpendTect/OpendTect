#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    virtual void	operator++()			= 0;

			/*!Force to skip progress info. */
    virtual void	skipProgress(bool yn)		{}
};


// mDeprecated: this inclusion will disappear after 6.0
#include "progressmeterimpl.h"
