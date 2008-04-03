#ifndef vissplittexturerandomline_h
#define vissplittexturerandomline_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		3-12-2008
 RCS:		$Id: vissplittexturerandomline.h,v 1.2 2008-04-03 19:13:16 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "visobject.h"

class SoSeparator;

namespace visBase
{

class Coordinates;
/*!Splitting a surface along random line into smaller blocks, where the random 
   line is a set of binids with some knots. Before having the shape, you have to   at least set the knots and depth range. */

class SplitTextureRandomLine : public VisualObjectImpl
{
public:
    static SplitTextureRandomLine*	create()
					mCreateDataObj(SplitTextureRandomLine);

    void				enableSpliting(bool yn);
    bool				isSplitingEnabled() const;
    void				setTextureUnits(const TypeSet<int>&);
    void				setTexturePath(const TypeSet<BinID>&,
						       int nrzpixels);

    void				setDepthRange(const Interval<float>&); 
    void				setLineKnots(const TypeSet<BinID>&);
    
    const Coordinates*			getCoordinates() const;
    Interval<float>			getDepthRange() const;
    mVisTrans* 				getDisplayTransformation();
    void				setDisplayTransformation(mVisTrans*);

protected:
    					~SplitTextureRandomLine();
    void				updateDisplay();

    TypeSet<BinID>			path_;
    TypeSet<BinID>			knots_;
    
    int					nrzpixels_;
    Interval<float>			zrg_;

    bool 				dosplit_;
    TypeSet<int>			usedunits_;

    Coordinates*			coords_;
    ObjectSet<SoSeparator>		separators_;
};

}; // Namespace


#endif
