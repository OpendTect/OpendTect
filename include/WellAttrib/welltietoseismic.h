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
namespace Well { class Data; }

namespace WellTie
{
    class Data;

mClass DataPlayer
{
public:
			DataPlayer(Data&,const MultiID&,const LineKey* lk=0);

    bool 		computeAll();
    bool		doFullSynthetics();
    bool		doFastSynthetics();
    bool		computeAdditionalInfo(const Interval<float>&);

    const char*		errMSG() const		{ return errmsg_.buf(); } 
   
protected:

    bool		extractSeismics();
    bool		setAIModel();
    bool		copyDataToLogSet();
    bool		processLog(const Well::Log*,Well::Log&,const char*); 
    void		createLog(const char*nm,float* dah,float* vals,int sz);

    ElasticModel 	aimodel_;
    ReflectivityModel	refmodel_;
    Data&		data_;

    const MultiID&	seisid_;
    const LineKey*	linekey_;
    TypeSet<float>	reflvals_;

    StepInterval<float> disprg_;
    StepInterval<float> workrg_;
    int			dispsz_;
    int			worksz_;

    BufferString	errmsg_;
    const Well::Data*	wd_;

    GeoCalculator 	geocalc_;
};

};//namespace WellTie
#endif
