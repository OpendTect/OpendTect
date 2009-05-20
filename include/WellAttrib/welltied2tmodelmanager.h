#ifndef welltied2tmodelmanager_h
#define welltied2tmodelmanager_h

/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltied2tmodelmanager.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "namedobj.h"
#include "welltiegeocalculator.h"

class MultiID;
class WellTieSetup;
class WellTieLogUnitFactors;
namespace Well
{
    class Data;
    class D2TModel;
}

/*!\brief Manages the D2T models used during TWTS. */

mClass WellTieD2TModelMGR
{
public:
				WellTieD2TModelMGR(Well::Data&,
						       const WellTieSetup&,
						       WellTieGeoCalculator&);
				~WellTieD2TModelMGR();

    bool 		save(const char* filenm);

    			// operations
    bool 		undo();
    bool 		cancel();

    bool      		updateFromWD();
    bool      		commitToWD();

    void 		shiftModel(float);
    void 		setAsCurrent(Well::D2TModel*);
    void		setFromVelLog(bool docln=true);
    void		setFromData(const Array1DImpl<float>&,
				    const Array1DImpl<float>&);

protected:

    Well::Data& 		wd_;
    Well::D2TModel& 		d2T();
    Well::D2TModel* 		prvd2t_;
    Well::D2TModel* 		orgd2t_;
    WellTieGeoCalculator&	geocalc_;
    const WellTieSetup&		wtsetup_;
    bool			emptyoninit_;
};

#endif
