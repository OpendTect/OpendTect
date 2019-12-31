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
	enum Type	    { ODv6, ODv7, Python };
				mDeclareEnumUtils(Type)
	enum ActionType     { Add, Remove, AddNRemove };
				mDeclareEnumUtils(ActionType)

				DataEntry() {}
				~DataEntry()
				{
				    int x = 0;
				}
	BufferString		execnm_;
	uiString		desc_;
	Type			type_;
	static const char*	sKeyODv6()	    { return "ODv6"; }
	static const char*	sKeyODv7()	    { return "ODv7"; }
	static const char*	sKeyPython()	    { return "Python"; }
	static const char*	sKeyActionTaken()   { return "ActionTaken"; }
    };

    mExpClass(Basic) Data : public ManagedObjectSet<DataEntry>
    {
    public:
				Data() { nrprocadded_ = 0; }
				~Data() {}
	Data&			add(DataEntry*);
	Data&			add(const char*,const uiString&,
							    DataEntry::Type);

	void			setPath(const BufferString&);
	DataEntry::ActionType	getActionType();

	const DataEntry*	get(const char*);
	void			getProcData(BufferStringSet&,uiStringSet&,
					const DataEntry::Type,
					const DataEntry::ActionType acttyp);
	IOPar&			readPars();
	bool			writePars(const IOPar&);

    protected:
	IOPar			pars_;
	BufferString		path_;
	BufferStringSet		addedodv6procs_;
	BufferStringSet		addedodv7procs_;
	BufferStringSet		addedpyprocs_;
	int			nrprocadded_;

	void			setEmpty();
	void			getProcsToBeAdded(BufferStringSet& nms,
				    uiStringSet& descs,const DataEntry::Type);
	void			getProcsToBeRemoved(BufferStringSet& nms,
				    uiStringSet& descs,const DataEntry::Type);
    };

}

mGlobal(Basic) ProcDesc::Data&		    ePDD();
mGlobal(Basic) const ProcDesc::Data&	    PDD();



#endif
