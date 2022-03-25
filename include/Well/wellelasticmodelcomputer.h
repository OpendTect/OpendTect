#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Arnaud Huck
 Date:		March 2013
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
    void		setLogs(const Log& vel, const Log* den=nullptr,
				const Log* svel=nullptr);
    void		setZrange(const Interval<float>&, bool istime);
    void		setExtractionPars(float step, bool intime);

    bool		computeFromLogs();
			/*!<Set at least the velocity log before */
    bool		computeFromDTModel();
			/*!<Logs won't be used - not yet implemented */
    const ElasticModel&	elasticModel() const	{ return emodel_; }


protected:

    void		init();
    bool		getLogUnits();
    bool		extractLogs();
    float		getLogVal(int logidx, int sampidx) const;
    float		getVelp(int) const;
    float		getDensity(int) const;
    float		getSVel(int) const;

    ElasticModel	emodel_;
    Interval<float>	zrange_;
    bool		zrgistime_;
    float		zstep_;
    bool		extractintime_;
    ConstRefMan<Data>	wd_;

    ObjectSet<const Log> inplogs_;
    ObjectSet<const UnitOfMeasure> uomset_;
    bool		velpissonic_;
    LogSampler*		ls_;
    LogSampler*		lsnearest_;

    uiString		errmsg_;
    uiString		warnmsg_;

};

}; // namespace Well


