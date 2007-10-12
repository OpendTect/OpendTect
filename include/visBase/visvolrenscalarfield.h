#ifndef visvolrenscalarfield_h
#define visvolrenscalarfield_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          January 2007
 RCS:           $Id: visvolrenscalarfield.h,v 1.6 2007-10-12 19:14:34 cvskris Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "ranges.h"
#include "visdata.h"

class SoGroup;
class SoTransferFunction;
class SoVolumeData;
class IOPar;
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

    void			useShading(bool yn) { useshading_=yn; }
    				//!<\note must be called before getInventorNode

    bool			turnOn(bool);
    bool			isOn() const;

    void			setScalarField(const Array3D<float>*);

    void			setColorTab(VisColorTab&);
    VisColorTab&		getColorTab();
    void			setBlendColor(const Color&);
    const Color&		getBlendColor() const;
    const TypeSet<float>&	getHistogram() const;

    void			setVolumeSize(const Interval<float>& x,
					      const Interval<float>& y,
					      const Interval<float>& z);
    Interval<float>		getVolumeSize(int dim) const;

    SoNode*			getInventorNode();

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

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
    bool			useshading_;
};

};

#endif
