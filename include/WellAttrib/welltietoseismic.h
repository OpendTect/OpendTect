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

#include "wellattribmod.h"
#include "ailayer.h"
#include "ranges.h"
#include "reflectivitymodel.h"
#include "welltiegeocalculator.h"

class LineKey;
class MultiID;
namespace Well { class Data; class D2TModel; }

namespace WellTie
{
    class Data;

mExpClass(WellAttrib) DataPlayer
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

    const Well::Data*	wd_;
    const Well::D2TModel* d2t_;
    ElasticModel 	aimodel_;
    ReflectivityModel	refmodel_;
    Data&		data_;
    GeoCalculator 	geocalc_;

    const MultiID&	seisid_;
    const LineKey*	linekey_;

    StepInterval<float> disprg_;
    StepInterval<float> workrg_;
    StepInterval<float> timerg_;
    int			dispsz_;
    int			worksz_;
    TypeSet<float>	reflvals_;

    BufferString	errmsg_;
};

};//namespace WellTie
#endif

