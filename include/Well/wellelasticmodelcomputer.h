#ifndef wellelasticmodelcomputer_h
#define wellelasticmodelcomputer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Arnaud Huck
 Date:		March 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellmod.h"
#include "ailayer.h"
#include "bufstringset.h"
#include "ranges.h"
#include "unitofmeasure.h"

namespace Well
{
class Data;
class Log;
class LogSet;
class LogSampler;

/*!
\extraction of an elastic model from well data
\Either from the time-depth model
\Or from at least a velocity log
*/

mExpClass(Well) ElasticModelComputer
{
public :
    			ElasticModelComputer(const Well::Data&);
			ElasticModelComputer(const Well::Data&,
			       		     const Well::Log& vel,
					     const Well::Log* den=0,
					     const Well::Log* svel=0);
			~ElasticModelComputer();

    bool		setVelLog(const Well::Log&);
    bool		setDenLog(const Well::Log&);
    bool		setSVelLog(const Well::Log&);
    void		setLogs(const Well::Log& vel, const Well::Log* den=0,
	    			const Well::Log* svel=0);
    void		setZrange(const Interval<float>&, bool istime);
    void		setExtractionPars(float step, bool intime);

    bool		computeFromLogs();
			/*!<\set at least the velocity log before */
    bool		computeFromDTModel();
    			/*!<\logs won't be used - not yet implemented */
    const ElasticModel&	elasticModel() const	{ return emodel_; }


protected:

    void		init();
    bool		getLogUnits();
    bool		extractLogs();
    float		getLogVal(int logidx, int sampidx) const;
    float		getVelp(int) const;
    float 		getDensity(int) const;
    float		getSVel(int) const;

    ElasticModel	emodel_;
    Interval<float>	zrange_;
    bool		zrgistime_;
    float		zstep_;
    bool		extractintime_;
    const Well::Data&	wd_;

    ObjectSet<const Well::Log> inplogs_;
    ObjectSet<const UnitOfMeasure> uomset_;
    bool		velpissonic_;
    Well::LogSampler*	ls_;
    Well::LogSampler*	lsnearest_;

    BufferString	errmsg_;

};

}; // namespace Well


#endif

