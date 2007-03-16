#ifndef visvolrenscalarfield_h
#define visvolrenscalarfield_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          January 2007
 RCS:           $Id: visvolrenscalarfield.h,v 1.3 2007-03-16 11:24:01 cvsnanne Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "ranges.h"
#include "visdata.h"

class SoTransferFunction;
class SoVolumeData;
class SoGroup;
template <class T> class Array3D;
template <class T> class ValueSeries;

namespace visBase
{
class VisColorTab;

class VolumeRenderScalarField : public DataObject
{
public:

    static VolumeRenderScalarField*	create()
	                        	mCreateDataObj(VolumeRenderScalarField);

    bool			turnOn(bool);
    bool			isOn() const;

    void			setScalarField(const Array3D<float>*);

    void			setColorTab(VisColorTab&);
    VisColorTab&		getColorTab();
    void			setBlendColor(const Color&);
    const Color&		getBlendColor() const;

    const TypeSet<float>&	getHistogram() const;

    void			setVolumeSize(	const Interval<float>& x,
	    					const Interval<float>& y,
						const Interval<float>& z );

    Interval<float>		getVolumeSize(int dim) const;

    SoNode*			getInventorNode();

protected:
    				~VolumeRenderScalarField();
    void			colorTabChCB(CallBacker*);
    void			colorSeqChCB(CallBacker*);
    void			autoscaleChCB(CallBacker*);
    void			makeColorTables();
    void			makeIndices( bool doset );
    void			clipData();

    SoGroup*			root_;
    SoTransferFunction*		transferfunc_;
    SoVolumeData*		voldata_;
    unsigned char		dummytexture_;

    VisColorTab*		ctab_; 

    int				sz0_, sz1_, sz2_;
    unsigned char*		indexcache_;
    bool			ownsindexcache_;
    const ValueSeries<float>*	datacache_;
    bool			ownsdatacache_;
    TypeSet<float>		histogram_;
    Color			blendcolor_;

    static visBase::FactoryEntry oldnameentry;
};

};

#endif
