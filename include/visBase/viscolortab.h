#ifndef viscolortab_h
#define viscolortab_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscolortab.h,v 1.1 2002-03-11 10:46:12 kristofer Exp $
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
    void		setColorSeq( ColorSequence* );

    const ColorSequence&	colorSeq() const { return *colseq; }

    Notifier<VisColorTab>	change;
    void			triggerChange() { change.trigger(); }
    				/*!< Use if yo have blocked change and need
				     to trigger it from outside
				 */
protected:
    			VisColorTab();
    virtual		~VisColorTab();

    void		colorseqchanged();

    ColorSequence*	colseq;
    LinScaler&		scale;
};

}; // Namespace


#endif
