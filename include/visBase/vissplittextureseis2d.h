#ifndef vissplittextureseis2d_h
#define vissplittextureseis2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		3-8-2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "posinfo2d.h"
#include "visobject.h"

class SoSeparator;
class SoIndexedTriangleStripSet;
class SoTextureCoordinate2;
class SoTextureComposer;

namespace visBase
{

class Coordinates;

/*!
\ingroup visBase
\brief Used for splitting 2D surface into smaller blocks with triangle strips.
  No matter split texture or not, we always split the shape along horizon with
  size mMaxHorSz. should set path before having the shape. To split texture,
  make sure to set z pixels and texture units.
*/

mExpClass(visBase) SplitTextureSeis2D : public VisualObjectImpl
{
public:
    static SplitTextureSeis2D*	create()
				mCreateDataObj(SplitTextureSeis2D);

    void			setTextureZPixelsAndPathScale(int zsz,int);
    				/*!<\note Horizontal size is trcrg.width()+1, 
					  but need scale if set resolution. */

    void			setPath(const TypeSet<PosInfo::Line2DPos>&);
    				//!<Is assumed to remain in memory

    void			setDisplayedGeometry(const Interval<int>& trcrg,
						     const Interval<float>& zrg);
    				//!<trcrg is indexes in path

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans* 		getDisplayTransformation() const;
    const Coordinates*		getCoordinates() const	{ return coords_; } 
    
protected:
    				~SplitTextureSeis2D();
    void			updateDisplay();
    void			updateHorSplit();
    void			updateSeparator(SoSeparator*,
	    				SoIndexedTriangleStripSet*&,
					SoTextureCoordinate2*&,
					SoTextureComposer*&,bool) const;
    TypeSet<Coord>		path_; 
    Interval<float>		zrg_;
    Interval<int>		trcrg_;
    int				nrzpixels_;
    int				horscale_;
    int				maxtexturesz_;
    ObjectSet<TypeSet<int> > 	horblocktrcindices_;

    Coordinates*		coords_;
    ObjectSet<SoSeparator>	separators_;
    TypeSet<int>		trcnrs_;
};

} // namespace visBase

#endif

