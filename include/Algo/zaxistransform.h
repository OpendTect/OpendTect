#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          October 2006
________________________________________________________________________

-*/

#include "algomod.h"
#include "enums.h"
#include "factory.h"
#include "binid.h"
#include "ranges.h"
#include "refcount.h"
#include "samplingdata.h"
#include "survgeom.h"

class BinIDValue;
class Scaler;
class TrcKeyZSampling;
class TaskRunner;

namespace ZDomain { class Def; class Info; }

/*!
\brief Base class for z-axis transforms.

  ZAxisTransform is the base class for z stretching in different ways.
  The z-stretch may be dependent on the location (binid).
  The various transforms can be retrieved from factory ZATF().
*/

mExpClass(Algo) ZAxisTransform
{ mRefCountImpl(ZAxisTransform);
public:
				mDefineFactoryInClass(ZAxisTransform,factory);

    static ZAxisTransform*	create(const IOPar&);
				/*!< Result will be reffed once. It is
				          caller's responsibility to unref. */

    virtual bool		isOK() const		{ return true; }
    virtual uiString		errMsg() const		{ return errmsg_; }

    virtual bool		needsVolumeOfInterest() const	{ return true; }
    virtual int			addVolumeOfInterest(const TrcKeyZSampling&,
						    bool zistrans=false);
				/*!<\returns id of new Volume of Interest.*/
    virtual void		setVolumeOfInterest(int volid,
						    const TrcKeyZSampling&,
						    bool zistrans=false);
    virtual void		removeVolumeOfInterest(int volid);

    virtual bool		loadDataIfMissing(int volid,TaskRunner* =0);

    virtual bool		canTransformSurv(OD::GeomSystem) const	= 0;

				//Generic 2D and 3D
    virtual void		transformTrc(const TrcKey&,
					  const SamplingData<float>&,
					 int sz,float* res) const	= 0;
    float			transformTrc(const TrcKey&,float z) const;
    virtual void		transformTrcBack(const TrcKey&,
					  const SamplingData<float>&,
					  int sz,float* res) const	= 0;
    float			transformTrcBack(const TrcKey&,float z) const;

    virtual Interval<float>	getZInterval(bool from) const		= 0;
				/*!\return the z interval in either to
				     or from domain. */
    virtual float		getZIntervalCenter(bool from) const;
				/*!\return a position within the
				    z-range that is a logical 'center' */
    virtual float		getGoodZStep() const;
				/*!\return a reasonable step in the
				    transformed domain. Default
				    implementation gives the same step as in
				    SI() (i.e. non transformed domain) */

    void			setVelUnitOfMeasure(const Scaler*);
    const Scaler*		getVelUnitOfMeasure() const;

    ZDomain::Info&		fromZDomainInfo() { return fromzdomaininfo_; }
    ZDomain::Info&		toZDomainInfo()	  { return tozdomaininfo_; }
    const ZDomain::Info&	fromZDomainInfo() const;
    const ZDomain::Info&	toZDomainInfo() const;
    const char*			fromZDomainKey() const;
    const char*			toZDomainKey() const;

    virtual float		toZScale() const;
				/*!<\returns the target domain z-scale. */
    virtual float		zScale() const { return toZScale(); }
				/*!<Old name, use toZScale instead. */

    virtual NotifierAccess*	changeNotifier()	{ return 0; }
    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:
				ZAxisTransform(const ZDomain::Def& from,
					       const ZDomain::Def& to);

    ZDomain::Info&		tozdomaininfo_;
    ZDomain::Info&		fromzdomaininfo_;
    mutable uiString		errmsg_;

private:
    const Scaler*		scaler_		= nullptr;

public: //Legacy stuff

				//3D
    virtual void		transform(const BinID&,
				      const SamplingData<float>&,
				      int sz,float* res) const;
    float			transform(const BinIDValue&) const;
    float			transform(const Coord3&) const;
    virtual void		transformBack(const BinID&,
					  const SamplingData<float>&,
					  int sz,float* res) const;
    float			transformBack(const BinIDValue&) const;
    float			transformBack(const Coord3&) const;
				// 2D
    virtual void		transform2D(const char* linenm,int trcnr,
					const SamplingData<float>&,
					int sz,float* res) const;
    float			transform2D(const char* linenm,int trcnr,
					float z) const;
    virtual void		transformBack2D(const char* linenm,int trcnr,
					    const SamplingData<float>&,
					    int sz,float* res) const;
    float			transformBack2D(const char* linenm,int trcnr,
					    float z) const;
};


/*!
\brief Samples Z-axis transformed data
*/

mExpClass(Algo) ZAxisTransformSampler
{
public:
				ZAxisTransformSampler(const ZAxisTransform&,
				   bool back,const SamplingData<double>&,
				   bool is2d);
    virtual			~ZAxisTransformSampler();

    void			setBinID(const BinID& bid);
    void			setTrcKey( const TrcKey& k ) { trckey_ = k ; }
    void			setLineName(const char*);
    void			setTrcNr(int);

    float			operator[](int idx) const;
    void			computeCache(const Interval<int>& range);

protected:

    const ZAxisTransform&	transform_;
    bool			back_;
    bool			is2d_;
    TrcKey			trckey_;
    const SamplingData<double>	sd_;

    TypeSet<float>		cache_;
    int				firstcachesample_;

};

