#ifndef visrandomtracksection_h
#define visrandomtracksection_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visrandomtrack.h,v 1.1 2003-01-07 10:34:18 kristofer Exp $
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

    void			setClipRate( float );
    float			clipRate() const;

    void			setAutoScale( bool yn );
    bool			autoScale() const;

    void			setColorTab( VisColorTab& );
    VisColorTab&		getColorTab();

    void			setData( int, const Array2D<float>& );

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

