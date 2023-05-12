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
#include "sets.h"

class BufferStringSet;
class DBKey;
class IOObj;
class Mnemonic;
class MnemonicSelection;
class MultiID;
class StringPairSet;
class UnitOfMeasure;

namespace Well
{
class Data;
class Log;

/*\brief Tells the Well Manager what you need to be loaded.*/

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
			~LoadReqs();

    static LoadReqs	All();
    static LoadReqs	AllNoLogs();
    bool		operator ==( const LoadReqs& oth ) const
						{ return reqs_ == oth.reqs_; }

    LoadReqs&		add(SubObjType);
    LoadReqs&		remove(SubObjType);
    LoadReqs&		setToAll();
    LoadReqs&		setEmpty();
    LoadReqs&		include(const LoadReqs&);
    LoadReqs&		exclude(const LoadReqs&);

    bool		isEmpty() const;
    bool		includes(SubObjType) const;
    bool		includes(const LoadReqs&) const;
    BufferString	toString() const;

protected:

    std::bitset<mWellNrSubObjTypes>		reqs_;
};



/*!
\brief Well manager
*/

mExpClass(Well) Man : public CallBacker
{
public:
			~Man();

    void		removeObject(const Data*);
    void		removeObject(const MultiID&);
    RefMan<Data>	get(const MultiID&);
    RefMan<Data>	get(const MultiID&,LoadReqs);
    RefMan<Data>	get(const DBKey&,LoadReqs);
    bool		isLoaded(const MultiID&) const;
    bool		reload(const MultiID&,LoadReqs lreq=LoadReqs(false));
    bool		reloadDispPars(const MultiID&, bool for2d=false);
    bool		reloadLogs(const MultiID&);
    bool		validID(const MultiID&) const;

    const char*		errMsg() const		{ return msg_; }
    WeakPtrSet<Data>&	wells()			{ return wells_; }

    bool		deleteLogs(const MultiID&,const BufferStringSet&);
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
				       bool onlyloaded=false);
    static bool		getAllMnemonics(MnemonicSelection&,
					bool onlyloaded=false);

    static bool		getMarkersByID(const MultiID&, BufferStringSet&);
    static bool		getMarkersByID(const MultiID&, BufferStringSet&,
				       TypeSet<OD::Color>&);
    static bool		getMarkersByID(const MultiID&, BufferStringSet&,
				       TypeSet<OD::Color>&, TypeSet<float>&);
    static bool		getLogNamesByID(const MultiID&,BufferStringSet&,
					bool onlyloaded=false);
    static void		getLogIDs(const MultiID&,const BufferStringSet&,
				  TypeSet<int>&);
    static void		getLogIDs(const MultiID&,const MnemonicSelection&,
				  TypeSet<int>&);
    Coord		getMapLocation(const MultiID&) const;

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

protected:

			Man();

    static Man*		mgr_;
    mGlobal(Well) friend Man&	MGR();

    WeakPtrSet<Data>	wells_;
    BufferString	msg_;
    TypeSet<MultiID>	allwellsids_;
    bool		isreloading_		= false;

    void		checkForUndeletedRef(CallBacker*);
    int			gtByKey(const MultiID&) const;
    RefMan<Data>	addNew(const MultiID&, LoadReqs lreq=LoadReqs(false));
    bool		readReqData(const MultiID&,Data&,LoadReqs);
    void		reloadAll();

    void		addFileSystemWatchCB(CallBacker*);
    void		wellDirChangedCB(CallBacker*);
    void		wellFilesChangedCB(CallBacker*);

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

} // namespace Well
