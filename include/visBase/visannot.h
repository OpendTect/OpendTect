#ifndef visannot_h
#define visannot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id$
________________________________________________________________________


-*/


#include "visbasemod.h"
#include "visobject.h"
#include "color.h"
#include "cubesampling.h"
#include "position.h"
#include "sets.h"

class SoAction;
class SbVec3f;
class SoSwitch;
class SoCallback;
class SoCoordinate3;
class SoIndexedLineSet;
class AxisInfo;
class Color;
class SoOneSideRender;

namespace osg { class Geode; }

namespace visBase
{
class Text2;
class DataObjectGroup;
class PickStyle;

/*!\brief
    Annotation is a simple wireframe box with text on its
    axis.
*/

mClass(visBase) Annotation : public VisualObjectImpl
{
public:
    static Annotation*		create()
				mCreateDataObj(Annotation);

    void			showText(bool yn);
    bool			isTextShown() const;

    void			showScale(bool yn);
    bool			isScaleShown() const;

    bool			canShowGridLines() const;
    void			showGridLines(bool yn);
    bool			isGridLinesShown() const;

    void			setCubeSampling(const CubeSampling&);

    void			setAnnotScale(int dim,int scale);

    void			setText( int dim, const char * );
    void			setTextColor(int dim,const Color&);
    const Color&		getColor()		{return annotcolor_;}
    void			updateTextColor(const Color&);

    void			fillPar( IOPar&, TypeSet<int>& ) const;
    int				usePar( const IOPar& );

protected:
    				~Annotation();
    void			initGridLines();
    void			updateGridLines();
    void			updateGridLines( int dim );
    void			updateTextPos(int dim);
    void			updateTextPos();
    Text2*			getText(int dim, int textnr);
    void			getAxisCoords(int,SbVec3f&,SbVec3f&) const;
    void			setCorner( int, float, float, float );
    Coord3			getCorner(int) const;

    SoCoordinate3*		coords_;
    int				annotscale_[3];

    ObjectSet<DataObjectGroup>	scales_;
    PickStyle*			pickstyle_;
    DataObjectGroup*		texts_;

    SoOneSideRender*		onesiderender_;
    SoSwitch*			gridlineswitch_; 
    SoCoordinate3*		gridlinecoords_;
    ObjectSet<SoIndexedLineSet>	gridlines_;

    SoSwitch*			textswitch_;
    SoSwitch*			scaleswitch_;
    osg::Geode*			geode_;
    Color			annotcolor_;

    static const char*		textprefixstr();
    static const char*		cornerprefixstr();
    static const char*		showtextstr();
    static const char*		showscalestr();
};

};

#endif

