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

/*!\brief Manages the D2T models used during TWTS. 
  CheckShot Correction are applied this way :
  Sonic->'transformed' CheckShot correction->Sonic->d2tm
  but
  Sonic->d2tm->direct CheckShot correction->d2tm->Sonic 
  would have been also possible.
  With the 1st method, we save a stage but we end up with a 
  d2tm that needs to be shifted from a first reference value 
  given by the CheckShot.
*/

namespace WellTie
{

class DataWriter;
class Params;

mClass D2TModelMGR
{
public:
			D2TModelMGR(WellTie::DataHolder&);
			~D2TModelMGR();

    			// operations
    bool 		undo();
    bool 		cancel();

    bool      		updateFromWD();
    bool      		commitToWD();

    void 		applyCheckShotShiftToModel();
    void 		replaceTime(const Array1DImpl<float>&);
    void 		shiftModel(float);
    void 		setAsCurrent(Well::D2TModel*);
    void		setFromVelLog(const char*,bool docln=true);
    void		setFromData(const Array1DImpl<float>&,
				    const Array1DImpl<float>&);

protected:

    DataHolder& 		dholder_;
    Well::D2TModel& 		d2T();
    Well::D2TModel* 		prvd2t_;
    Well::D2TModel* 		orgd2t_;
    WellTie::GeoCalculator&	geocalc_;
    const WellTie::Params&	params_;
    const WellTie::DataWriter*	datawriter_;
    bool			emptyoninit_;

    Well::Data* 		wd();

    void			ensureValid(Well::D2TModel&);
};

}; //namespace WellTie

#endif
