#ifndef vissplittexturerandomline_h
#define vissplittexturerandomline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		3-12-2008
 RCS:		$Id: vissplittexturerandomline.h,v 1.5 2009-07-22 16:01:25 cvsbert Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "visobject.h"

class SoSeparator;
class SoIndexedTriangleStripSet;
class SoTextureCoordinate2;
class SoTextureComposer;


namespace visBase
{

class Coordinates;
/*!Splitting a surface along random line into smaller blocks, where the random 
   line is a set of binids with some knots. Before having the shape, you have to   at least set the knots and depth range. */

mClass SplitTextureRandomLine : public VisualObjectImpl
{
public:
    static SplitTextureRandomLine*	create()
					mCreateDataObj(SplitTextureRandomLine);

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
    void				updateSeparator(SoSeparator*, 
	    					SoIndexedTriangleStripSet*&,
						SoTextureCoordinate2*&,
						SoTextureComposer*&,bool) const;

    TypeSet<BinID>			path_;
    TypeSet<BinID>			knots_;
    
    int					nrzpixels_;
    Interval<float>			zrg_;

    Coordinates*			coords_;
    ObjectSet<SoSeparator>		separators_;
};

}; // Namespace


#endif
