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

#include "welltiegeocalculator.h"

class AIModel;
class LineKey;
class MultiID;
class TaskRunner;

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

    void 		computeAll();
    void		generateSynthetics();
   
protected:

    void		extractSeismics();
    void		resetAIModel();
    void		copyDataToLogSet();

    void		createLog(const char*,const TypeSet<float>&,
				    const TypeSet<float>&);

    AIModel*		aimodel_;
    Data&		data_;
    const MultiID&	seisid_;
    const LineKey*	linekey_;
    int			refsz_;

    GeoCalculator 	geocalc_;
};

};//namespace WellTie
#endif
