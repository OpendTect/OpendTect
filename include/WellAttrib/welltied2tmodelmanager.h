#ifndef welltied2tmodelmanager_h
#define welltied2tmodelmanager_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltied2tmodelmanager.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "namedobj.h"
#include "welltiegeocalculator.h"

class MultiID;
namespace Well
{
    class Data;
    class D2TModel;
}

/*!\brief Manages the D2T models used during TWTS. */

namespace WellTie
{

class Setup;
class LogUnitFactors;

mClass D2TModelMGR
{
public:
				D2TModelMGR(Well::Data*,
					   const WellTie::Params*);
				~D2TModelMGR();

    bool 		save(const char* filenm);

    			// operations
    bool 		undo();
    bool 		cancel();

    bool      		updateFromWD();
    bool      		commitToWD();

    void 		replaceTime(const Array1DImpl<float>&);
    void 		shiftModel(float);
    void 		setAsCurrent(Well::D2TModel*);
    void		setFromVelLog(const char*,bool docln=true);
    void		setFromData(const Array1DImpl<float>&,
				    const Array1DImpl<float>&);

protected:

    Well::Data* 		wd_;
    Well::D2TModel& 		d2T();
    Well::D2TModel* 		prvd2t_;
    Well::D2TModel* 		orgd2t_;
    WellTie::GeoCalculator&	geocalc_;
    const WellTie::Params*	params_;
    const WellTie::Setup&	wtsetup_;
    bool			emptyoninit_;
};

}; //namespace WellTie

#endif
