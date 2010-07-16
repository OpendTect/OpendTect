#ifndef wellt2dtransform_h
#define wellt2dtransform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id: wellt2dtransform.h,v 1.2 2010-07-16 10:23:28 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "zaxistransform.h"

#include "multiid.h"
#include "welldata.h"

class IOPar;

namespace Well
{

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
    const char*			getToZDomainString() const;
    const char*			getZDomainID() const;
    bool			fillPar(IOPar&);
    bool			usePar(const IOPar&);
    bool			needsVolumeOfInterest() const
				{ return false; }

protected:
    bool			calcDepths();

    MultiID			mid_;
    Well::Data*			data_;
    TypeSet<float>		times_;
    TypeSet<float>		depths_;
};

} //namespace

#endif
