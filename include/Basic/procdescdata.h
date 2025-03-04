#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

#include "bufstring.h"
#include "enums.h"
#include "manobjectset.h"

class BufferStringSet;
class uiString;

namespace ProcDesc
{

    mExpClass(Basic) DataEntry
    {
    public:
	enum Type	    { OD, Python };
				mDeclareEnumUtils(Type)
	enum ActionType     { Add, Remove, AddNRemove };
				mDeclareEnumUtils(ActionType)

				DataEntry();
				~DataEntry();
	BufferString		execnm_;
	uiString		desc_;
	Type			type_;
	static const char*	sKeyODv6()	    { return "ODv6"; }
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
	bool			hasWorkToDo(const BufferString& pypath,bool);

    protected:
	IOPar			pars_;
	BufferString		path_;
	BufferStringSet		addedodprocs_;
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
mGlobal(Basic) void			    gatherFireWallProcInf();
