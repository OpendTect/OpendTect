#ifndef viscolortab_h
#define viscolortab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscolortab.h,v 1.16 2007-10-16 05:07:16 cvsraman Exp $
________________________________________________________________________


-*/

#include "viscolorseq.h"

template <class T> class Interval;
template <class T> class ValueSeries;
class LinScaler;

namespace visBase
{

/*!\brief


*/

class VisColorTab : public DataObject
{
public:
    static VisColorTab*		create()
				mCreateDataObj(VisColorTab);

    bool			autoScale() const;
    void			setAutoScale(bool yn);
    float			clipRate() const;
    void			setClipRate(float);
    bool			getSymmetry() const;
    void			setSymmetry(bool yn);
    void			setSymmetrical(Interval<float>&);
    void			scaleTo(const Interval<float>& rg);
    void			scaleTo(const float* values, int nrvalues);
    				/*!< Does only work if autoscale is true */
    void			scaleTo(const ValueSeries<float>& values,
	    				int nrvalues);
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

    void			colorseqchanged();

    ColorSequence*		colseq_;
    LinScaler&			scale_;

    bool			autoscale_;
    float			cliprate_;
    bool			symmetry_;

    static const char*		sKeyColorSeqID();
    static const char*		sKeyScaleFactor();
    static const char*		sKeyClipRate();
    static const char*		sKeyAutoScale();
};

}; // Namespace


#endif
