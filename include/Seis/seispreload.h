#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			PreLoader(const MultiID&,Pos::GeomID=Pos::GeomID::udf(),
				  TaskRunner* =nullptr);
			~PreLoader();

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
				Scaler* =nullptr) const;
    bool		load(const TypeSet<TrcKeyZSampling>&,
			     const TypeSet<Pos::GeomID>&,
				DataCharacteristics::UserType=
					DataCharacteristics::Auto,
				Scaler* =nullptr) const;
    bool		loadPS3D(const Interval<int>* inlrg=nullptr) const;
    bool		loadPS2D(const char* lnm=nullptr) const;
			//!< null => all
    bool		loadPS2D(const BufferStringSet&) const;

    void		unLoad() const;
    uiString		errMsg() const			{ return errmsg_; }

    static void		load(const IOPar&,TaskRunner* =nullptr);
			//!< Seis.N.[loadObj_fmt]
    static void		loadObj(const IOPar&,TaskRunner* =nullptr);
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
			PreLoadDataEntry(const DataPack&,const MultiID&,
					 Pos::GeomID);
			~PreLoadDataEntry();

    bool		equals(const MultiID&,Pos::GeomID) const;
    DataPackID	dpID() const;

    ConstRefMan<DataPack> getDP() const;
    RefMan<DataPack>	getDP();

    MultiID		mid_;
    Pos::GeomID		geomid_;
    bool		is2d_;
    BufferString	name_;

private:

    ConstRefMan<DataPack> dp_;
    DataPackMgr&	dpmgr_;
};


mExpClass(Seis) PreLoadDataManager : public CallBacker
{
public:

    void		add(const DataPack&,const MultiID&);
    void		add(const DataPack&,const MultiID&,Pos::GeomID);
    void		remove(const MultiID&,Pos::GeomID=Pos::GeomID::udf());
    void		remove(const DataPackID&);

    template<class T>
    inline ConstRefMan<T>	get(DataPackID) const;
    template<class T>
    inline ConstRefMan<T>	get(const MultiID&,
				    Pos::GeomID=Pos::GeomID::udf()) const;

    ConstRefMan<DataPack>	getDP(DataPackID) const;
    ConstRefMan<DataPack>	getDP(const MultiID&,
				      Pos::GeomID=Pos::GeomID::udf()) const;

    void			getInfo(const MultiID&,Pos::GeomID,
					BufferString&) const;

    void			getIDs(TypeSet<MultiID>&) const;
    bool			isPresent(const MultiID&,
					  Pos::GeomID=Pos::GeomID::udf()) const;

    const ObjectSet<PreLoadDataEntry>& getEntries() const;

    Notifier<PreLoadDataManager>	changed;

private:

    void		surveyChangeCB(CallBacker*);

    ManagedObjectSet<PreLoadDataEntry> entries_;

public:
			PreLoadDataManager();
			~PreLoadDataManager();

};

mGlobal(Seis) PreLoadDataManager& PLDM();


template <class T> inline ConstRefMan<T>
PreLoadDataManager::get( const MultiID& mid, Pos::GeomID gid ) const
{
    auto dp = getDP( mid, gid );
    mDynamicCastGet( const T*, casted, dp.ptr() );
    return ConstRefMan<T>( casted );
}

template <class T> inline ConstRefMan<T>
PreLoadDataManager::get( DataPackID dpid ) const
{
    auto dp = getDP( dpid );
    mDynamicCastGet( const T*, casted, dp.ptr() );
    return ConstRefMan<T>( casted );
}

} // namespace Seis
