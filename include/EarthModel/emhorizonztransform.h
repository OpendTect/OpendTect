#ifndef emhorizonztransform_h
#define emhorizonztransform_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		April 2006
 RCS:		$Id: emhorizonztransform.h,v 1.1 2007-01-15 14:45:21 cvskris Exp $
________________________________________________________________________


-*/

#include "zaxistransform.h"

namespace EM
{
class Horizon;

/*!Z-transform that flatterns a horizon. Everything else will also be flatterned
accordingly. In case of reverse faulting, the area between the two patches will
not be included.  */

class HorizonZTransform : public ZAxisTransform
			, public CallBacker
{
public:
    			HorizonZTransform(const Horizon&);
    void		setHorizon(const Horizon&);
    ZType		getFromZType() const;
    ZType		getToZType() const { return ZAxisTransform::Other; }
    void		transform(const BinID&,const SamplingData<float>&,
				  int sz,float* res) const;
    void		transformBack(const BinID&,const SamplingData<float>&,
				  int sz,float* res) const;

    Interval<float>	getZInterval(bool from) const;

protected:
    			~HorizonZTransform();
    void		calculateHorizonRange();
    void		horChangeCB( CallBacker* );
    bool		getTopBottom(const BinID&,float&top,float&bottom) const;

    const Horizon*	horizon_;
    Interval<float>	depthrange_;
    bool		horchanged_;
};


}; // Namespace


#endif
