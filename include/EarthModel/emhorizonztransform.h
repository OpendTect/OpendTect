#ifndef emhorizonztransform_h
#define emhorizonztransform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		April 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "zaxistransform.h"

namespace EM
{
class Horizon;

/*!Z-transform that flatterns a horizon. Everything else will also be flatterned
accordingly. In case of reverse faulting, the area between the two patches will
not be included.  */

mClass HorizonZTransform : public ZAxisTransform
			 , public CallBacker
{
public:
    mDefaultFactoryInstantiation( ZAxisTransform, HorizonZTransform,
				  "HorizonZTransform", sFactoryKeyword() );

    static const char*	sKeyHorizonID()		{ return "Horizon"; }

    			HorizonZTransform();
    void		setHorizon(const Horizon&);
    void		transform(const BinID&,const SamplingData<float>&,
				  int sz,float* res) const;
    void		transformBack(const BinID&,const SamplingData<float>&,
				  int sz,float* res) const;

    Interval<float>	getZInterval(bool from) const;
    float		getZIntervalCenter(bool from) const;
    bool		needsVolumeOfInterest() const	{ return false; }

    Interval<float>	getDepthRange() const		{ return depthrange_; }

    int			lineIndex(const char* lnm) const;
    NotifierAccess*	changeNotifier()		{ return &change_; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    			~HorizonZTransform();
    void		calculateHorizonRange();
    void		horChangeCB( CallBacker* );
    bool		getTopBottom(const BinID&,float&top,float&bottom) const;

    const Horizon*	horizon_;
    Interval<float>	depthrange_;
    bool		horchanged_;
    Notifier<HorizonZTransform> change_;

};

} // namespace EM

#endif
