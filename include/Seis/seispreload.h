#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2009
________________________________________________________________________


-*/

#include "seiscommon.h"
#include "datachar.h"
#include "datapackbase.h"
#include "dbkey.h"
#include "ranges.h"
#include "survgeom.h"
#include "taskrunner.h"

class IOObj;
class Scaler;
class TrcKeyZSampling;

namespace Seis
{

mExpClass(Seis) PreLoader
{ mODTextTranslationClass(PreLoader);
public:

    mUseType( Pos,	GeomID );

			PreLoader(const DBKey&,GeomID gid=GeomID::get3D(),
				TaskRunner* =0);

    const DBKey&	id() const			{ return dbkey_; }
    GeomID		geomID() const			{ return geomid_; }
    void		setTaskRunner( TaskRunner& t )	{ tr_ = &t; }

    IOObj*		getIOObj() const;
    Interval<int>	inlRange() const;
			//!< PS 3D only. If nothing there: ret.start==mUdf(int)
    void		getLineNames(BufferStringSet&) const;
			//!< Line 2D only.

    bool		load(const TrcKeyZSampling&,
				DataCharacteristics::UserType=OD::AutoDataRep,
				const Scaler* =0) const;
    bool		load(const TypeSet<TrcKeyZSampling>&,const GeomIDSet&,
			     DataCharacteristics::UserType=OD::AutoDataRep,
			     const Scaler* =0) const;
    bool		loadPS3D(const Interval<int>* inlrg=0) const;
    bool		loadPS2D(const char* lnm=0) const;	//!< null => all
    bool		loadPS2D(const BufferStringSet&) const;

    void		unLoad() const;
    uiString		errMsg() const			{ return errmsg_; }

    static void		load(const IOPar&,TaskRunner* tskr=0);
			//!< Seis.N.[loadObj_fmt]
    static void		loadObj(const IOPar&,TaskRunner* tskr=0);
			//!< sKey::ID() and optional subselections
    void		fillPar(IOPar&) const;

    static const char*	sKeyLines();
    static const char*	sKeyUserType();

protected:

    DBKey		dbkey_;
    GeomID		geomid_;
    TaskRunner*		tr_;
    SilentTaskRunner	deftr_;
    mutable uiString	errmsg_;

    TaskRunner&		getTr() const
			{ return *((TaskRunner*)(tr_ ? tr_ : &deftr_)); }
};


mExpClass(Seis) PreLoadDataEntry
{
public:

    mUseType( Pos,	GeomID );

			PreLoadDataEntry(const DBKey&,GeomID,
					 DataPack::ID);

    bool		equals(const DBKey&,GeomID) const;

    DBKey		dbkey_;
    GeomID		geomid_;
    DataPack::ID	dpid_;
    bool		is2d_;
    BufferString	name_;
};


mExpClass(Seis) PreLoadDataManager
{
public:

    mUseType( Pos,		GeomID );
    typedef DataPack::ID	PackID;

    void			add(const DBKey&,DataPack*);
    void			add(const DBKey&,GeomID,DataPack*);
    void			remove(const DBKey&,GeomID gid=GeomID());
    void			remove(PackID);
    void			removeAll();

    template<class T>
    inline RefMan<T>		get(const DBKey&,GeomID gid=GeomID());
    template<class T>
    inline RefMan<T>		get(PackID);
    template<class T>
    inline ConstRefMan<T>	get(const DBKey&,GeomID gid=GeomID()) const;
    template<class T>
    inline ConstRefMan<T>	get(PackID) const;

    void			getInfo(const DBKey&,GeomID,
					BufferString&) const;

    void			getIDs(DBKeySet&) const;
    bool			isPresent(const DBKey&,
					  GeomID gid=GeomID()) const;

    const ObjectSet<PreLoadDataEntry>& getEntries() const;

protected:

    DataPackMgr&			dpmgr_;
    ManagedObjectSet<PreLoadDataEntry>	entries_;

public:

				PreLoadDataManager();
				~PreLoadDataManager();

    RefMan<DataPack>		getDP(const DBKey&,GeomID gid=GeomID());
    inline RefMan<DataPack>	getDP( DataPack::ID dpid )
				{ return dpmgr_.getDP( dpid ); }
    ConstRefMan<DataPack>	getDP(const DBKey&,GeomID gid=GeomID()) const;
    inline ConstRefMan<DataPack> getDP( DataPack::ID dpid ) const
				{ return dpmgr_.getDP( dpid ); }

};

mGlobal(Seis) PreLoadDataManager& PLDM();


template <class T> inline RefMan<T>
PreLoadDataManager::get( const DBKey& dbky, GeomID gid )
{
    auto dp = getDP( dbky, gid );
    mDynamicCastGet( T*, casted, dp.ptr() );
    return RefMan<T>( casted );
}

template <class T> inline ConstRefMan<T>
PreLoadDataManager::get( const DBKey& dbky, GeomID gid ) const
{
    auto dp = getDP( dbky, gid );
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
