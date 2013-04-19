#ifndef welltietoseismic_h
#define welltietoseismic_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltietoseismic.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "ailayer.h"
#include "ranges.h"
#include "reflectivitymodel.h"
#include "welltiegeocalculator.h"

class LineKey;
class MultiID;
namespace Well { class Data; class LogSet;}

namespace WellTie
{
    class Data;

mClass DataPlayer
{
public:
			DataPlayer(Data&,const MultiID&,const LineKey* lk=0);

    bool 		computeAll();
    // do not use, will be removed

    bool 		computeSynthetics();
    bool		extractSeismics();
    bool		doFastSynthetics();
    bool		computeAdditionalInfo(const Interval<float>&);
    bool		computeElasticModelFromLogs(ElasticModel&,
	    				const StepInterval<float>,bool rgistime,					const Well::Data&, const Well::LogSet&,
					BufferString& errmsg);
    // do not use, will be removed
    bool		isOKSynthetic() const;
    bool		isOKSeismic() const;
    bool		hasSeisId() const;

    const char*		errMSG() const		{ return errmsg_.buf(); } 
   
protected:

    bool		setAIModel();
    bool		doFullSynthetics();
    // do not use, will be removed
    bool		copyDataToLogSet();
    bool		processLog(const Well::Log*,Well::Log&,const char*); 
    void		createLog(const char*nm,float* dah,float* vals,int sz);

    ElasticModel 	aimodel_;
    ReflectivityModel	refmodel_;
    Data&		data_;
    const MultiID&	seisid_;
    const LineKey*	linekey_;
    TypeSet<float>	reflvals_;
	// do not use, will be removed

    StepInterval<float> disprg_;
    // do not use, will be removed
    StepInterval<float> workrg_;
    // do not use, will be removed
    int			dispsz_;
    // do not use, will be removed
    int			worksz_;
    // do not use, will be removed

    BufferString	errmsg_;
    const Well::Data*	wd_;
    // do not use, will be removed

    GeoCalculator 	geocalc_;
    // do not use, will be removed
};

};//namespace WellTie
#endif

