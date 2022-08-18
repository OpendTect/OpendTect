#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
