#ifndef wellt2dtransform_h
#define wellt2dtransform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "wellmod.h"
#include "zaxistransform.h"

class IOPar;

namespace Well { class Data; }

/*!
\brief Time to depth transform for wells.
*/

mExpClass(Well) WellT2DTransform : public ZAxisTransform
{
public:

    mDefaultFactoryInstantiation( ZAxisTransform, WellT2DTransform,
				  "WellT2D", sFactoryKeyword() );

				WellT2DTransform();

    void			transform(const BinID&,
	    				  const SamplingData<float>&,
					  int sz,float* res) const;
    void			transformBack(const BinID&,
	    				      const SamplingData<float>&,
					      int sz,float* res) const;

    Interval<float>		getZInterval(bool time) const;
    bool			needsVolumeOfInterest() const { return false; }

    bool			usePar(const IOPar&);

protected:

    Well::Data*			data_;
    TypeSet<float>		times_;
    TypeSet<float>		depths_;

    bool			calcDepths();
    void			doTransform(const BinID&,
	    				    const SamplingData<float>&,
					    int sz,float*,bool) const;
};

#endif

