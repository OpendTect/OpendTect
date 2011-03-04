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

#include "ranges.h"
#include "reflectivitymodel.h"
#include "welltiegeocalculator.h"

class AIModel;
class LineKey;
class MultiID;
class AILayer;
class RayTracer1D;
class TaskRunner;
namespace Well { class Data; }

template <class T> class TypeSet;

namespace WellTie
{
    class Data;
    class Setup;

mClass DataPlayer
{
public:
			DataPlayer(Data&,const MultiID&,const LineKey* lk=0);
			~DataPlayer();

    bool 		computeAll();
    bool		generateSynthetics();

    const char*		errMsg() const		{ return errmsg_.buf(); } 
   
protected:

    bool		extractSeismics();
    bool		setAIModel();
    bool		setD2TModel();
    bool		setRefModel();
    bool		runRayTracer();
    bool		copyDataToLogSet();
    bool		processLog(const Well::Log*,
				    Well::Log&,const char*); 
    void		createLog(const char*,const TypeSet<float>&,
				    const TypeSet<float>&);

    TypeSet<AILayer> 	aimodel_;
    ReflectivityModel	refmodel_;
    Data&		data_;

    const MultiID&	seisid_;
    const LineKey*	linekey_;
    RayTracer1D*	raytracer_;
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
