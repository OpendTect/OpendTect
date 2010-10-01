#ifndef visvolrenscalarfield_h
#define visvolrenscalarfield_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          January 2007
 RCS:           $Id: visvolrenscalarfield.h,v 1.12 2010-10-01 08:10:00 cvskarthika Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "ranges.h"
#include "visdata.h"
#include "coltabmapper.h"
#include "coltabsequence.h"


class TaskRunner;
class SoGroup;
class SoTransferFunction;
class SoVolumeData;
class IOPar;
template <class T> class Array3D;
template <class T> class ValueSeries;

namespace visBase
{

mClass VolumeRenderScalarField : public DataObject
{
public:

    static VolumeRenderScalarField*	create()
	                        	mCreateDataObj(VolumeRenderScalarField);

    void			useShading(bool yn) { useshading_=yn; }
    				//!<\note must be called before getInventorNode

    bool			turnOn(bool);
    bool			isOn() const;

    void			setScalarField(const Array3D<float>*,bool mine,
	    				       TaskRunner*);

    void			setColTabSequence( const ColTab::Sequence&,
	    					   TaskRunner* tr );
    const ColTab::Sequence&	getColTabSequence();

    void			setColTabMapperSetup(const ColTab::MapperSetup&,
	    					   TaskRunner* tr );
    const ColTab::Mapper&	getColTabMapper();

    void			setBlendColor(const Color&);
    const Color&		getBlendColor() const;
    const TypeSet<float>&	getHistogram() const;

    void			setVolumeSize(const Interval<float>& x,
					      const Interval<float>& y,
					      const Interval<float>& z);
    Interval<float>		getVolumeSize(int dim) const;

    SoNode*			getInventorNode();

    const char*			writeVolumeFile(std::ostream&) const;
				//!<\returns 0 on success, otherwise errmsg

    virtual int			usePar(const IOPar&);

protected:
    				~VolumeRenderScalarField();

    void			makeColorTables();
    void			makeIndices(bool doset,TaskRunner*);
    void			clipData(TaskRunner*);

    SoGroup*			root_;
    SoTransferFunction*		transferfunc_;
    SoVolumeData*		voldata_;
    unsigned char		dummytexture_;

    ColTab::Sequence		sequence_;
    ColTab::Mapper		mapper_;

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
