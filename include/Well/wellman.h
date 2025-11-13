#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include <bitset>
#include "bufstring.h"
#include "bufstringset.h"
#include "color.h"
#include "callback.h"
#include "ptrman.h"
#include "refcount.h"
#include "uistring.h"

class BufferStringSet;
class DBKey;
class IOObj;
class Mnemonic;
class MnemonicSelection;
class MultiID;
class StringPairSet;
class UnitOfMeasure;
class WellFileList;
class Timer;


namespace Well
{
class Data;
class Log;

/*\brief Tells the Well Manager what needs to be loaded.*/

#define mWellNrSubObjTypes 9

enum SubObjType		{ Inf=0, Trck=1, D2T=2, CSMdl=3, Mrkrs=4, Logs=5,
			   LogInfos=6, DispProps2D=7, DispProps3D=8 };


mExpClass(Well) LoadReqs
{
public:

			LoadReqs(bool addall=true);
			LoadReqs(SubObjType);
			LoadReqs(SubObjType,SubObjType);
			LoadReqs(SubObjType,SubObjType,SubObjType);
    explicit		LoadReqs(const char*)			= delete;
			LoadReqs(const OD::String& lognm);
			LoadReqs(const BufferStringSet& lognms);
			~LoadReqs();

    static LoadReqs	getLoadReqFromFileExt(const char*);
    static LoadReqs	All();
    static LoadReqs	AllNoLogs(); //!< But with LogInfos
    bool		operator ==(const LoadReqs&) const;
    bool		operator !=(const LoadReqs&) const;

    LoadReqs&		add(SubObjType);
    LoadReqs&		addLog(const char* lognm);
    LoadReqs&		addLogs(const BufferStringSet&);
    LoadReqs&		remove(SubObjType);
    LoadReqs&		setToAll();
    LoadReqs&		setEmpty();
    LoadReqs&		include(const LoadReqs&);
    LoadReqs&		exclude(const LoadReqs&);
    LoadReqs&		excludeLogSel();
    LoadReqs&		allowMissingLogs(bool yn);

    bool		isEmpty() const;
    bool		includes(SubObjType) const;
    bool		includes(const LoadReqs&) const;
    bool		allowMissingLogs() const { return allowmissinglogs_; }
    BufferString	toString() const;

    const BufferStringSet& logNames() const	{ return lognms_; }

protected:

    std::bitset<mWellNrSubObjTypes>		reqs_;
    BufferStringSet	lognms_;
    bool		allowmissinglogs_	= false;
};



/*!
\brief Well manager
*/

mExpClass(Well) Man : public CallBacker
{
mODTextTranslationClass(Man)
public:
			~Man();

    void		cleanupNullPtrs();
    void		removeObject(const Data*);
    void		removeObject(const MultiID&);
    RefMan<Data>	get(const MultiID&);
    RefMan<Data>	get(const MultiID&,const LoadReqs&);
    RefMan<Data>	get(const DBKey&,const LoadReqs&);
    bool		isLoaded(const MultiID&) const;
    LoadReqs		loadState(const MultiID&) const;
    bool		reload(const MultiID&,LoadReqs =LoadReqs(false));
    bool		reloadDispPars(const MultiID&,bool for2d=false);
    bool		reloadLogs(const MultiID&);
			//!< Limited to the already loaded logs
    bool		validID(const MultiID&) const;

    uiString		errMsg() const		{ return errmsg_; }
    WeakPtrSet<Data>&	wells()			{ return wells_; }

    bool		deleteLogs(const MultiID&,const BufferStringSet&);
    bool		deleteMarkers(const MultiID&,const BufferStringSet&);
    static bool		renameLog(const TypeSet<MultiID>&,const char* oldnm,
							  const char* newnm);
    static bool		getWellKeys(TypeSet<MultiID>&,bool onlyloaded=false);
    static bool		getWellNames(BufferStringSet&,bool onlyloaded=false);
    static bool		getAllMarkerNames(BufferStringSet&,
					  bool onlyloaded=false);
    static bool		getAllMarkerNames(BufferStringSet&,
					  const RefObjectSet<const Data>&);
    static bool		getAllMarkerInfo(BufferStringSet&,
					 TypeSet<OD::Color>&,
					 bool onlyloaded=false);
    static bool		getAllLogNames(BufferStringSet&,
				       bool onlyloadedwells=false);
    static bool		getAllMnemonics(MnemonicSelection&,
					bool onlyloadedwells=false);

    static bool		getMarkersByID(const MultiID&,BufferStringSet&);
    static bool		getMarkersByID(const MultiID&,BufferStringSet&,
				       TypeSet<OD::Color>&);
    static bool		getMarkersByID(const MultiID&, BufferStringSet&,
				       TypeSet<OD::Color>&,TypeSet<float>&);
    static bool		getLogNamesByID(const MultiID&,BufferStringSet&);
    static void		getLogIDs(const MultiID&,const BufferStringSet&,
				  TypeSet<int>&);
    static void		getLogIDs(const MultiID&,const MnemonicSelection&,
				  TypeSet<int>&);
    Coord		getMapLocation(const MultiID&) const;

    uiRetVal		writeLogHeaders(const MultiID&,
					const BufferStringSet& lognms);
    bool		writeAndRegister(const MultiID&,PtrMan<Log>&);
					//!< Log becomes mine
    bool		writeAndRegister(const MultiID&,ObjectSet<Log>&);
					//!< Returns empty set if all succeeded
    bool		isReloading() const;
    static void		dumpMgrInfo(StringPairSet&);

    static const BufferString	wellDirPath();

    const Mnemonic*	getMnemonicOfLog(const char* lognm) const;

    static const UnitOfMeasure*	surveyDepthStorageUnit();
    static const UnitOfMeasure*	surveyDepthDisplayUnit();

    CNotifier<Well::Man,const TypeSet<MultiID>&>	wellsAdded;
    CNotifier<Well::Man,const TypeSet<MultiID>&>	wellsRemoved;

protected:

			Man();

    static Man*			mgr_;
    mGlobal(Well) friend Man&	MGR();

    WeakPtrSet<Data>		wells_;
    uiString			errmsg_;
    bool			isreloading_		= false;
    int				wellgrpid_;

    void		checkForUndeletedRef(CallBacker*);
    int			gtByKey(const MultiID&) const;
    RefMan<Data>	addNew(const MultiID&,LoadReqs =LoadReqs(false));
    bool		readReqData(const MultiID&,const LoadReqs&,Data&);
    void		reloadAll();

    void		wellAddedToDB(CallBacker*);
    void		wellsAddedToDB(CallBacker*);
    void		wellEntryRemovedCB(CallBacker*);
    void		wellEntriesRemovedCB(CallBacker*);

    static const UnitOfMeasure*	depthstorageunit_;
    static const UnitOfMeasure*	depthdisplayunit_;

public:
    mDeprecated("Use getLogNamesByID instead")
    static bool		getLogNames(const MultiID&,BufferStringSet&,
				    bool forceLoad=false);
    mDeprecated("Use getAllMarkerNames instead")
    static bool		getMarkerNames(BufferStringSet&);

};

mGlobal(Well) Man& MGR();

mGlobal(Well) IOObj* findIOObj(const char* wellnm,const char* uwi);
mGlobal(Well) float displayToStorageDepth(float);
mGlobal(Well) float storageToDisplayDepth(float);

/*!\ use creafully, will rewrite the database entry.
Rule-of-thumb: If IOM().implExists(id), then consider whether you really
need it.*/
mGlobal(Well) bool	putUWI(const MultiID&,const char* uwi);
mGlobal(Well) bool	putUWIs(const ObjectSet<std::pair<const MultiID,
				const char*>>&);

} // namespace Well
