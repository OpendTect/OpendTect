#ifndef procdescdata_h
#define procdescdata_h

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : September 2019
-*/

#include "basicmod.h"

#include "bufstring.h"
#include "enums.h"
#include "factory.h"
#include "objectset.h"

class BufferStringSet;
class uiString;

namespace ProcDesc
{
    mExpClass(Basic) DataEntry
    {
    public:
				enum Type { OD, Python };
				    mDeclareEnumUtils(Type)
				DataEntry() {}
				~DataEntry() {}
	BufferString		execnm_;
	uiString		desc_;
	Type			type_;
    };

    mExpClass(Basic) Data : public ManagedObjectSet<DataEntry>
    {
    public:
			    Data() {}
			    ~Data() {}
	Data&		    add(DataEntry*);
	Data&		    add(const char*,const uiString&,DataEntry::Type);

	const DataEntry*    get(const char*);
	void		    getProcData(BufferStringSet&,uiStringSet&,
					const DataEntry::Type);
    };

}

mGlobal(Basic) ProcDesc::Data&		    ePDD();
mGlobal(Basic) const ProcDesc::Data&	    PDD();



#endif
