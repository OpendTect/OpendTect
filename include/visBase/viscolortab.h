#ifndef viscolortab_h
#define viscolortab_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscolortab.h,v 1.3 2002-03-14 08:50:18 nanne Exp $
________________________________________________________________________


-*/

#include "viscolorseq.h"

template <class T> class Interval;
class LinScaler;

namespace visBase
{

/*!\brief


*/

class VisColorTab : public DataObject
{
public:
    static VisColorTab*	create()
			mCreateDataObj0arg(VisColorTab);

    Color		color( float val ) const;
    void		scaleTo( const Interval<float>& rg );
    Interval<float>	getInterval() const;

    void		setColorSeq( ColorSequence* );

    const ColorSequence&	colorSeq() const { return *colseq; }
    ColorSequence&		colorSeq() { return *colseq; }

    Notifier<VisColorTab>	change;
    void			triggerChange() { change.trigger(); }
protected:
    			VisColorTab();
    virtual		~VisColorTab();

    void		colorseqchanged();

    ColorSequence*	colseq;
    LinScaler&		scale;
};

}; // Namespace


#endif
