#pragma once

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
				}
	BufferString		execnm_;
	uiString		desc_;
	Type			type_;
	static const char*	sKeyODv6()	    { return "ODv6"; }
	static const char*	sKeyODv7()	    { return "ODv7"; }
	static const char*	sKeyPython()	    { return "Python"; }
	static const char*	sKeyActionTaken()   { return "ActionTaken"; }
	static const char*	getCMDActionKey(const ActionType);
	static ActionType	getActionTypeForCMDKey(const BufferString&);
	static const char*	getTypeFlag(const Type);
	static bool		isCMDOK(const BufferString&);
    };

    mExpClass(Basic) Data : public ManagedObjectSet<DataEntry>
    {
    public:
				Data() {}
				~Data() {}
	Data&			add(DataEntry*);
	Data&			add(const char*,const uiString&,
							    DataEntry::Type);

	void			setPath(const BufferString&);
	DataEntry::ActionType	getActionType();

	void			getProcData(BufferStringSet&,uiStringSet&,
					const DataEntry::Type,
					const DataEntry::ActionType acttyp);
	IOPar&			readPars();
	bool			writePars(const IOPar&,bool toadd);
	static BufferString	sKeyODExecNm() { return "od_main"; }
	bool			hasWorkToDo(const BufferString& pypath,bool);

    protected:
	IOPar			pars_;
	BufferString		path_;
	BufferStringSet		addedodv6procs_;
	BufferStringSet		addedodv7procs_;
	BufferStringSet		addedpyprocs_;
	BufferStringSet		addedprocnms_;

	void			setEmpty();
	void			getProcsToBeAdded(BufferStringSet& nms,
				    uiStringSet& descs,const DataEntry::Type);
	void			getProcsToBeRemoved(BufferStringSet& nms,
				    uiStringSet& descs,const DataEntry::Type);
    };

}

mGlobal(Basic) ProcDesc::Data&		    ePDD();
mGlobal(Basic) const ProcDesc::Data&	    PDD();



