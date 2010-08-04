#ifndef wellt2dtransform_h
#define wellt2dtransform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id: wellt2dtransform.h,v 1.3 2010-08-04 13:30:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "zaxistransform.h"

class IOPar;

namespace Well { class Data; }

mClass WellT2DTransform : public ZAxisTransform
{
public:

    static const char*		sName();
    const char*			name() const	{ return sName(); }
    static void			initClass();
    static ZAxisTransform*	create();

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
    bool			calcDepths();

    Well::Data*			data_;
    TypeSet<float>		times_;
    TypeSet<float>		depths_;
};

#endif
