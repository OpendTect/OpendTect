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
#include "bufstringset.h"
#include "ranges.h"
#include "reflectivitymodel.h"
#include "unitofmeasure.h"
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

    bool 		computeSynthetics();
    bool		extractSeismics();
    bool		doFastSynthetics();
    bool		computeAdditionalInfo(const Interval<float>&);
    			// log order in log set: Vp, [den], [Vs]
    bool		computeElasticModelFromLogs(ElasticModel&,
	    				const StepInterval<float>,bool rgistime,
					const Well::Data&,
					const ObjectSet<const Well::Log>&,
					BufferString& errmsg);
    bool		isOKSynthetic() const;
    bool		isOKSeismic() const;
    bool		hasSeisId() const;

    const char*		errMSG() const		{ return errmsg_.buf(); } 
   
protected:

    bool		setAIModel();
    bool		doFullSynthetics();
    bool		copyDataToLogSet();
    bool		processLog(const Well::Log*,Well::Log&,const char*); 
    void		createLog(const char*nm,float* dah,float* vals,int sz);

    ElasticModel 	aimodel_;
    ReflectivityModel	refmodel_;
    Data&		data_;
    const MultiID&	seisid_;
    const LineKey*	linekey_;

    BufferString	errmsg_;
};

};//namespace WellTie
#endif

