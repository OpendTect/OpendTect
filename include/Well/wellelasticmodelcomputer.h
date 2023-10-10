#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"
#include "ailayer.h"
#include "bufstringset.h"
#include "ranges.h"
#include "uistring.h"
#include "unitofmeasure.h"
#include "welldata.h"

namespace Well
{
class Log;
class LogSampler;

/*!
\brief Extraction of an ElasticModel from Well::Data either from the time-depth
model or from at least a velocity Log.
*/

mExpClass(Well) ElasticModelComputer
{ mODTextTranslationClass(ElasticModelComputer);
public :
			ElasticModelComputer(const Data&);
			ElasticModelComputer(const Data&,
					     const Log& vel,
					     const Log* den=nullptr,
					     const Log* svel=nullptr);
			~ElasticModelComputer();

    const uiString&	errMsg() const		{ return errmsg_; }
    const uiString&	warnMsg() const		{ return warnmsg_; }

    bool		setVelLog(const Log&);
    bool		setDenLog(const Log&);
    bool		setSVelLog(const Log&);
    void		setLogs(const Log& vel,const Log* den=nullptr,
				const Log* svel=nullptr);
    void		setZrange(const Interval<float>& zrg,bool istime);
			//!<\param zrg must be in SI units (meters or seconds).
    void		setExtractionPars(float step,bool intime);
			//!<\param step must be in SI units (meters or seconds).

    bool		computeFromLogs();
			/*!<Set at least the velocity log before */
    bool		computeFromDTModel();
			/*!<Logs won't be used - not yet implemented */
    const ElasticModel&	elasticModel() const	{ return emodel_; }


protected:

    bool		getLogUnits();
    bool		extractLogs();
    float		getLogVal(int logidx, int sampidx) const;
    float		getVelp(int) const;
    float		getDensity(int) const;
    float		getSVel(int) const;

    ElasticModel	emodel_;
    Interval<float>	zrange_ = Interval<float>::udf();
    bool		zrgistime_ = false;
    float		zstep_ = mUdf(float);
    bool		extractintime_ = false;
    ConstRefMan<Data>	wd_;

    ObjectSet<const Log> inplogs_;
    ObjectSet<const UnitOfMeasure> uomset_;
    bool		velpissonic_ = false;
    LogSampler*		ls_ = nullptr;
    LogSampler*		lsnearest_ = nullptr;

    uiString		errmsg_;
    uiString		warnmsg_;

};

} // namespace Well
