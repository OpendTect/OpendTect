#ifndef picksetsaver_h
#define picksetsaver_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "autosaver.h"


namespace Pick
{
class Set;

/*!\brief Saveable for Pick::Set. */

mExpClass(General) SetSaver : public OD::AutoSaveable
{
public:

			SetSaver(const Pick::Set&);

    const Pick::Set&	pickSet() const
			{ return static_cast<const Pick::Set&>( monitored() ); }

    virtual BufferString getFingerPrint() const;

protected:

    virtual bool	doStore(const IOObj&) const;

};

} // namespace Pick


#endif
