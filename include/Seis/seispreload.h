#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2009
________________________________________________________________________


-*/

#include "seismod.h"
#include "bufstringset.h"
#include "datachar.h"
#include "datapackbase.h"
#include "multiid.h"
#include "ranges.h"
#include "task.h"

class IOObj;
class Scaler;
class TrcKeyZSampling;

namespace Seis
{

mExpClass(Seis) PreLoader
{ mODTextTranslationClass(PreLoader)
public:
			PreLoader(const MultiID&,Pos::GeomID =-1,
				  TaskRunner* =0);

    const MultiID&	id() const			{ return mid_; }
    Pos::GeomID		geomID() const			{ return geomid_; }
    void		setTaskRunner( TaskRunner& t )	{ tr_ = &t; }

    IOObj*		getIOObj() const;
    Interval<int>	inlRange() const;
    			//!< PS 3D only. If nothing there: ret.start==mUdf(int)
    void		getLineNames(BufferStringSet&) const;
    			//!< Line 2D only.

    bool		load(const TrcKeyZSampling&,
				DataCharacteristics::UserType=
					DataCharacteristics::Auto,
				Scaler* =0) const;
    bool		load(const TypeSet<TrcKeyZSampling>&,
			     const TypeSet<Pos::GeomID>&,
				DataCharacteristics::UserType=
					DataCharacteristics::Auto,
				Scaler* =0) const;
    bool		loadPS3D(const Interval<int>* inlrg=0) const;
    bool		loadPS2D(const char* lnm=0) const;	//!< null => all
    bool		loadPS2D(const BufferStringSet&) const;

    void		unLoad() const;
    uiString		errMsg() const			{ return errmsg_; }

    static void		load(const IOPar&,TaskRunner* tr=0);
    			//!< Seis.N.[loadObj_fmt]
    static void		loadObj(const IOPar&,TaskRunner* tr=0);
    			//!< sKey::ID() and optional subselections
    void		fillPar(IOPar&) const;

    static const char*	sKeyLines();
    static const char*	sKeyUserType();

protected:

    MultiID		mid_;
    Pos::GeomID		geomid_;
    TaskRunner*		tr_;
    TaskRunner		deftr_;
    mutable uiString	errmsg_;

    TaskRunner&		getTr() const
    			{ return *((TaskRunner*)(tr_ ? tr_ : &deftr_)); }
};


mExpClass(Seis) PreLoadDataEntry
{
public:
    typedef	DataPack::ID	PackID;
			PreLoadDataEntry(const MultiID&,Pos::GeomID,PackID);

    bool		equals(const MultiID&,Pos::GeomID) const;

    MultiID		mid_;
    Pos::GeomID		geomid_;
    PackID		dpid_;
    bool		is2d_;
    BufferString	name_;
};


mExpClass(Seis) PreLoadDataManager : public CallBacker
{
public:
    typedef	DataPack::ID	PackID;

    void		add(const MultiID&,DataPack*);
    void		add(const MultiID&,Pos::GeomID,DataPack*);
    void		remove(const MultiID&,Pos::GeomID =-1);
    void		remove(PackID dpid);
    void		removeAll();

    template<class T>
    inline RefMan<T>		get(const MultiID&,Pos::GeomID =-1);
    template<class T>
    inline RefMan<T>		get(PackID dpid);
    template<class T>
    inline ConstRefMan<T>	get(const MultiID&,Pos::GeomID =-1) const;
    template<class T>
    inline ConstRefMan<T>	get(PackID dpid) const;

    void			getInfo(const MultiID&,Pos::GeomID,
					BufferString&) const;

    void			getIDs(TypeSet<MultiID>&) const;
    bool			isPresent(const MultiID&,Pos::GeomID =-1) const;

    const ObjectSet<PreLoadDataEntry>& getEntries() const;
    RefMan<DataPack>		getDP(const MultiID&,Pos::GeomID = -1);
    ConstRefMan<DataPack>	getDP(const MultiID&,Pos::GeomID = -1) const;
    inline RefMan<DataPack>	getDP( PackID dpid )
				{ return dpmgr_.getDP( dpid ); }
    inline ConstRefMan<DataPack> getDP( DataPack::ID dpid ) const
				{ return dpmgr_.getDP( dpid ); }

    Notifier<PreLoadDataManager>	changed;

protected:

    DataPackMgr&	dpmgr_;
    ManagedObjectSet<PreLoadDataEntry> entries_;

public:
			PreLoadDataManager();
			~PreLoadDataManager();
};

mGlobal(Seis) PreLoadDataManager& PLDM();


template <class T> inline RefMan<T>
PreLoadDataManager::get( const MultiID& mid, Pos::GeomID gid )
{
    auto dp = getDP( mid, gid );
    mDynamicCastGet( T*, casted, dp.ptr() );
    return RefMan<T>( casted );
}

template <class T> inline ConstRefMan<T>
PreLoadDataManager::get( const MultiID& mid, Pos::GeomID gid ) const
{
    auto dp = getDP( mid, gid );
    mDynamicCastGet( const T*, casted, dp.ptr() );
    return ConstRefMan<T>( casted );
}

template <class T> inline RefMan<T>
PreLoadDataManager::get( PackID dpid )
{
    auto dp = getDP( dpid );
    mDynamicCastGet( T*, casted, dp.ptr() );
    return RefMan<T>( casted );
}

template <class T> inline ConstRefMan<T>
PreLoadDataManager::get( PackID dpid ) const
{
    auto dp = getDP( dpid );
    mDynamicCastGet( const T*, casted, dp.ptr() );
    return ConstRefMan<T>( casted );
}

} // namespace Seis

