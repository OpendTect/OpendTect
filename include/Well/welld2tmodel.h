#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "welldahobj.h"
#include "uistring.h"

class TimeDepthModel;
class uiString;

namespace Well
{

/*!\brief Depth to time model. */

mExpClass(Well) D2TModel : public DahObj
{ mODTextTranslationClass(D2TModel);
public:

    typedef ValueType		TWTType;
    typedef TWTType		VelType;
    typedef ValueIntvType	TWTIntvType;
    typedef ValueSetType	TWTSetType;

			D2TModel(const char* nm=0);
			~D2TModel();
			mDeclMonitorableAssignment(D2TModel);
			mDeclInstanceCreatedNotifierAccess(D2TModel);

    virtual void	getData(ZSetType&,TWTSetType&) const;
    mImplSimpleMonitoredGetSet(inline,desc,setDesc,BufferString,
				desc_,cParsChange());
    mImplSimpleMonitoredGetSet(inline,dataSource,setDataSource,BufferString,
				datasource_,cParsChange());

    TWTType		getTime(ZType,const Track&) const;
    ZType		getDepth(TWTType,const Track&) const;
    ZType		getDah(TWTType,const Track&) const;
    double		getVelocityForDah(ZType,const Track&) const;
    double		getVelocityForDepth(ZType,const Track&) const;
    double		getVelocityForTwt(TWTType,const Track&) const;
    bool		getTimeDepthModel(const Well::Data&,
					  TimeDepthModel&) const;

    void		fillHdrPar(IOPar&) const;
    void		useHdrPar(const IOPar&);

    inline ValueType	t( PointID id ) const	{ return value(id); }

    static const char*	sKeyTimeWell(); //!< for well that is only known in time
    static const char*	sKeyDataSrc();

    void		setData(const ZSetType&,const ValueSetType&);
    void		makeFromTrack(const Track&,VelType cstvel,
				      VelType replvel);
			//!< cstvel: velocity of the TD model
			//!< replvel: Replacement velocity, above SRD
    bool		ensureValid(const Well::Data&,uiString& errmsg,
				    TypeSet<double>* zvals=0,
				    TypeSet<double>* tvals=0);
			//!< Returns corrected model if necessary
			//!< May eventually also correct info().replvel
    bool		calibrateBy(const D2TModel&);
			//!< returns whether any change was made

protected:

    TWTSetType		times_;
    BufferString	desc_;
    BufferString	datasource_;

    virtual bool	doSet(idx_type,ValueType);
    virtual PointID	doInsAtDah( ZType dh, ValueType val )
			{ return doIns( dh, val, times_, true  ); }
    virtual ValueType	gtVal( idx_type idx ) const  { return times_[idx]; }
    virtual void	removeAux( int idx )	    { times_.removeSingle(idx);}
    virtual void	eraseAux()		    { times_.erase(); }

    ZType		gtDepth(float,const Track&) const;
    double		gtVelocityForTwt(float,const Track&) const;
    bool		gtVelocityBoundsForDah(ZType d_ah,const Track&,
					  Interval<double>& depths,
					  Interval<float>& times) const;
    bool		gtVelocityBoundsForTwt(float twt,const Track&,
					  Interval<double>& depths,
					  Interval<float>& times) const;
			/*!<Gives index of next dtpoint at or after dah.*/
    int			gtVelocityIdx(ZType pos,const Track&,
				       bool posisdah=true) const;

protected:

    inline ZType	getDepth( float time ) const { return mUdf(ZType); }
			//!< Legacy, misleading name. Use getDah().
    bool		getOldVelocityBoundsForDah(ZType d_ah,const Track&,
					     Interval<double>& depths,
					     Interval<float>& times) const;
			//!<Read legacy incorrect time-depth model.
    bool		getOldVelocityBoundsForTwt(float twt,const Track&,
					     Interval<double>& depths,
					     Interval<float>& times) const;
			//!<Read legacy incorrect time-depth model.
    static bool getTVDD2TModel(Well::D2TModel&,const Well::Data&,
			  TypeSet<double>& zvals, TypeSet<double>& tvals,
			  uiString& errmsg, uiString& warnmsg);
    static void convertDepthsToMD(const Well::Track&,
				  const TypeSet<double>& zvals,ZSetType&);
    static void shiftTimesIfNecessary(TypeSet<double>& tvals, double wllheadz,
				 double vrepl, double origintwtinfile,
				  uiString& msg);
    static void checkReplacementVelocity(Well::Info&,double vreplinfile,
					 uiString& msg);

    friend class	D2TModelIter;
};


/*!\brief Well D2T Model iterator. */

mExpClass(Well) D2TModelIter : public DahObjIter
{
public:

    typedef D2TModel::TWTType	TWTType;

			D2TModelIter(const D2TModel&,bool start_at_end=false);
			D2TModelIter(const D2TModelIter&);

    const D2TModel&	model() const;
    TWTType		t() const;

};


} // namespace Well
