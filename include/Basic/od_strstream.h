#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2013
________________________________________________________________________

-*/

#include "basicmod.h"
#include "od_iostream.h"


/*!\brief OD class for streaming from string */

mExpClass(Basic) od_istrstream : public od_istream
{
public:

    			od_istrstream(const char*);

    const char*		input() const;

    void		setInput(const char*);

};


/*!\brief OD class for stream write into string */

mExpClass(Basic) od_ostrstream : public od_ostream
{
public:

    			od_ostrstream();

    const char*		result() const;

    void		setEmpty();

};


