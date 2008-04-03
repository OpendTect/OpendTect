#ifndef vissplittextureseis2d_h
#define vissplittextureseis2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		3-8-2008
 RCS:		$Id: vissplittextureseis2d.h,v 1.2 2008-04-03 19:11:50 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "posinfo.h"
#include "visobject.h"

class SoSeparator;

namespace visBase
{

class Coordinates;

/*!used for splitting 2D surface into smaller blocks with triangle strips.
   No matter split texture or not, we always split the shape along horizon with
   size mMaxHorSz. should set path before having the shape. To split texture,
   make sure to set z pixels and texture units. */

class SplitTextureSeis2D : public VisualObjectImpl
{
public:
    static SplitTextureSeis2D*	create()
				mCreateDataObj(SplitTextureSeis2D);

    void			enableSpliting(bool yn);
    bool			isSplitingEnabled() const;
    void			setTextureUnits(const TypeSet<int>&);
    void			setTextureZPixels(int);
    				//!<\note Horizontal size is trcrg.width()+1, 

    void			setPath(const TypeSet<PosInfo::Line2DPos>&);
    				//!<Is assumed to remain in memory

    void			setDisplayedGeometry(const Interval<int>& trcrg,
						    const Interval<float>& zrg);

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans* 			getDisplayTransformation();
    const Coordinates*		getCoordinates() const	{ return coords_; } 
    
protected:
    				~SplitTextureSeis2D();
    void			updateDisplay();
    void			updateHorSplit();

    bool 			splittexture_;
    TypeSet<int>		usedunits_;

    TypeSet<Coord>		path_; 
    Interval<float>		zrg_;
    Interval<int>		trcrg_;
    int				nrzpixels_;
    int				firsttrcnr_;
    ObjectSet<TypeSet<int> > 	horblocktrcindices_;

    Coordinates*		coords_;
    ObjectSet<SoSeparator>	separators_;
};

}; // Namespace


#endif
