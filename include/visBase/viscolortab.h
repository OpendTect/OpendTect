#ifndef viscolortab_h
#define viscolortab_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "viscolorseq.h"
#include "color.h"
#include "ranges.h"

namespace ColTab
{
    class Sequence;
    class Mapper;
    class IndexedLookUpTable;
}

template <class T> class ValueSeries;

namespace visBase
{

/*!\brief


*/

mClass VisColorTab : public DataObject
{
public:
    static VisColorTab*		create()
				mCreateDataObj(VisColorTab);

    bool			autoScale() const;
    void			setAutoScale(bool yn);
    Interval<float>		clipRate() const;
    void			setClipRate(Interval<float>);
    float			symMidval() const;
    void			setSymMidval(float);
    void			scaleTo(const Interval<float>& rg);
    void			scaleTo(const float* values, od_int64 nrvalues);
    				/*!< Does only work if autoscale is true */
    void			scaleTo(const ValueSeries<float>* values,
	    				od_int64 nrvalues);
    				/*!< Does only work if autoscale is true */
    Interval<float>		getInterval() const;

    void			setNrSteps(int);
    int				nrSteps() const;
    int				colIndex(float val) const;
    				/*!< return 0-nrSteps()-1 nrSteps() = undef; */
    Color			tableColor(int idx) const;
    Color			color(float) const;

    void			setColorSeq(ColorSequence*);
    const ColorSequence&	colorSeq() const;
    ColorSequence&		colorSeq();
    const ColTab::Mapper&	colorMapper() const	{ return *ctmapper_; }
    ColTab::Mapper&		colorMapper()		{ return *ctmapper_; }

    Notifier<VisColorTab>	rangechange;
    Notifier<VisColorTab>	sequencechange;
    Notifier<VisColorTab>	autoscalechange;

    void			triggerRangeChange();
    void			triggerSeqChange();
    void			triggerAutoScaleChange();

    int				usePar(const IOPar&);
    void			fillPar(IOPar&,TypeSet<int>&) const;

protected:
    virtual			~VisColorTab();

    void			colorseqChanged(CallBacker*);

    ColTab::Mapper*		ctmapper_;
    ColTab::IndexedLookUpTable*	indextable_;
    ColorSequence*		viscolseq_;

    static const char*		sKeyColorSeqID();
    static const char*		sKeyScaleFactor();
    static const char*		sKeyClipRate();
    static const char*		sKeyRange();
    static const char*		sKeyAutoScale();
    static const char*		sKeySymmetry();
    static const char*		sKeySymMidval();
};

}; // Namespace


#endif
