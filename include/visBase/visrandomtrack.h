#ifndef visrandomtracksection_h
#define visrandomtracksection_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visrandomtrack.h,v 1.3 2003-01-20 08:37:47 kristofer Exp $
________________________________________________________________________


-*/

#include "position.h"
#include "ranges.h"
#include "visobject.h"

class SoRandomTrackLineDragger;
template <class T> class Array2D;


namespace visBase
{
class TriangleStripSet;
class VisColorTab;

/*!\brief

*/

class RandomTrack : public VisualObjectImpl
{
public:
    static RandomTrack*		create()
				mCreateDataObj(RandomTrack);

    void			showDragger( bool yn );
    bool			isDraggerShown() const;

    int				nrKnots() const;
    void			addKnot(const Coord& );
    void			insertKnot( int pos, const Coord& );
    Coord			getKnotPos(int) const;
    void			setKnotPos(int, const Coord& );
    void			removeKnot( int );

    void			setDepthInterval( const Interval<float>& );
    const Interval<float>&	getDepthInterval() const;

    void			setXrange( const StepInterval<float>& );
    void			setYrange( const StepInterval<float>& );
    void			setZrange( const StepInterval<float>& );
    				/*!< sets limits for dragging */

    void			setClipRate( float );
    float			clipRate() const;

    void			setAutoScale( bool yn );
    bool			autoScale() const;

    void			setColorTab( VisColorTab& );
    VisColorTab&		getColorTab();

    void			setData( int section, const Array2D<float>& );
    				/*!< section ranges from 0 to nrKnots-2 */

protected:
    				~RandomTrack();
    void			rebuild();

    Interval<float>		depthrg;
    TypeSet<Coord>		knots;

    ObjectSet<TriangleStripSet>	sections;
    SoRandomTrackLineDragger*	dragger;
    SoSwitch*			draggerswitch;
};

};

#endif

