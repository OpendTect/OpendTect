#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellmod.h"
#include "sets.h"
#include "bufstring.h"
#include "color.h"
#include <bitset>

class DBKey;
class IOObj;
class MultiID;
class BufferStringSet;
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
    static LoadReqs	All();
    bool		operator ==( const LoadReqs& oth ) const
						{ return reqs_ == oth.reqs_; }

    LoadReqs&		add(SubObjType);
    LoadReqs&		remove( SubObjType typ ) { reqs_[typ]=0; return *this; }
    void		setToAll()		{ *this = All(); }
    void		setEmpty()		{ reqs_.reset(); }
    bool		isEmpty() const		{ return reqs_.none(); }
    void		include(const LoadReqs&);
    void		exclude(const LoadReqs&);

    bool		includes( SubObjType typ ) const
						{ return reqs_[typ]; }
    bool		includes(const LoadReqs&) const;
    BufferString	toString() const;

protected:

    std::bitset<mWellNrSubObjTypes>		reqs_;
};



/*!
\brief Well manager
*/

mExpClass(Well) Man
{
public:
			~Man();

    void		cleanup();
    void		removeObject(const Data*);
    void		removeObject(const MultiID&);
    Data*		get(const MultiID&);
    Data*		get(const MultiID&,LoadReqs);
    Data*		get(const DBKey&,LoadReqs);
    bool		readReqData(const MultiID&,Data*,LoadReqs);
    bool		isLoaded(const MultiID&) const;
    bool		reload(const MultiID&,LoadReqs lreq=LoadReqs(false));
    bool		reloadDispPars(const MultiID&, bool for2d=false);
    bool		reloadLogs(const MultiID&);
    bool		validID(const MultiID&) const;

    const char*		errMsg() const		{ return msg_; }
    ObjectSet<Data>&	wells()			{ return wells_; }

    bool		deleteLogs(const MultiID&,const BufferStringSet&);
    static bool		renameLog(const TypeSet<MultiID>&,const char* oldnm,
							  const char* newnm);
    static bool		getWellKeys(TypeSet<MultiID>&,bool onlyloaded=false);
    static bool		getWellNames(BufferStringSet&,bool onlyloaded=false);
    static bool		getAllMarkerNames(BufferStringSet&,
					  bool onlyloaded=false);
    static bool		getAllMarkerInfo(BufferStringSet&,
					 TypeSet<OD::Color>&,
				  bool onlyloaded=false);
    static bool		getAllLogNames(BufferStringSet&,
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
    Coord		getMapLocation(const MultiID&) const;

    bool		writeAndRegister(const MultiID&,const Log&);
					//!< Log becomes mine
    bool		writeAndRegister(const MultiID&,ObjectSet<Log>&);
					//!< Returns empty set if all succeeded
    static void		dumpMgrInfo(IOPar&);

    static const UnitOfMeasure*	surveyDepthStorageUnit();
    static const UnitOfMeasure*	surveyDepthDisplayUnit();

protected:

			Man()				{}
    static Man*		mgr_;
    mGlobal(Well) friend Man&	MGR();

    ObjectSet<Data>	wells_;
    BufferString	msg_;

    int			gtByKey(const MultiID&) const;
    Well::Data*		addNew(const MultiID&, LoadReqs lreq=LoadReqs(false));

    static const UnitOfMeasure*	depthstorageunit_;
    static const UnitOfMeasure*	depthdisplayunit_;

public:
    mDeprecated("Use getLogNamesByID instead")
    static bool		getLogNames(const MultiID&,BufferStringSet&,
				    bool forceLoad=false);
    mDeprecated("Use getAllMarkerNames instead")
    static bool		getMarkerNames(BufferStringSet&);

    mDeprecated("Use get instead")
    void		add(const MultiID&,Data*); //!< Data becomes mine

    mDeprecated("Use removeObject instead")
    Data*		release(const MultiID&); //!< Data becomes yours

};

mGlobal(Well) Man& MGR();

mGlobal(Well) IOObj* findIOObj(const char* wellnm,const char* uwi);
mGlobal(Well) float displayToStorageDepth(float);
mGlobal(Well) float storageToDisplayDepth(float);

} // namespace Well

