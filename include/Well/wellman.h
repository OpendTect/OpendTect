#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellcommon.h"
#include "sets.h"
#include "uistring.h"

class IOObj;
class DBKey;
class BufferStringSet;

namespace Well
{

/*!\brief Well manager */

mExpClass(Well) Man
{
public:
			~Man();

    void		removeObject( const Well::Data* );
    Data*		get(const DBKey&);
    void		add(const DBKey&,Data*);
			//!< Data becomes mine
    Data*		release(const DBKey&);
			//!< Data becomes yours
    bool		isLoaded(const DBKey&) const;
    bool		reload(const DBKey&);

    const uiString&	errMsg() const		{ return msg_; }
    ObjectSet<Data>&	wells()			{ return wells_; }

    static bool		getLogNames(const DBKey&,BufferStringSet&);
    static bool		getMarkerNames(BufferStringSet&);

protected:

			Man()				{}
    static Man*		mgr_;
    mGlobal(Well) friend Man&	MGR();

    ObjectSet<Data>	wells_;
    uiString		msg_;

    int			gtByKey(const DBKey&) const;
};

mGlobal(Well) Man& MGR();

mGlobal(Well) IOObj* findIOObj(const char* wellnm,const char* uwi);

} // namespace Well

