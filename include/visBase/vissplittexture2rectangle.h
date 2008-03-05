#ifndef vissplittexture2rectangle_h
#define vissplittexture2rectangle_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		2-28-2008
 RCS:		$Id: vissplittexture2rectangle.h,v 1.1 2008-03-05 19:29:14 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "visobject.h"

class SoSplitTexture2Part;
class SoTextureCoordinate2;
class SoIndexedFaceSet;

namespace visBase
{

class Coordinates;
/*!Rectangle shape that is divided into smaller parts, where each part
   has its own (small) texture, derived from a MultiTexture2.
   Object must be notified of MultiTexture2's size, splitting status and 
   used texture units. */

class SplitTexture2Rectangle : public VisualObjectImpl
{
public:
    static SplitTexture2Rectangle* create()
			mCreateDataObj(SplitTexture2Rectangle);

    void		enableSpliting(bool yn);
    bool		isSplitingEnabled() const  { return dosplit_; }
    
    void		setUsedTextureUnits(const TypeSet<int>&);
    			/*!<only necessary for splitting. get it from
			    your texture with getUsedTextureUnits(). */

    void		setOriginalTextureSize(int sz0,int sz1);

    void		setPosition(const Coord3& c00,const Coord3& c01,
				    const Coord3& c10,const Coord3& c11);
    const Coord3&	getPosition(bool dim0,bool dim1) const;
    
    mVisTrans* 		getDisplayTransformation();
    void		setDisplayTransformation(mVisTrans*);

protected:
    			~SplitTexture2Rectangle();

    static int		nrBlocks(int totalnr,int maxnr,int overlap); 
    void		updateFaceSets(); 
    void		updateCoordinates(); 

    TypeSet<float>	c00factors_;
    TypeSet<float>	c01factors_;
    TypeSet<float>	c10factors_;
    TypeSet<float>	c11factors_;
    TypeSet<int>	usedunits_;

    bool 		dosplit_;
    int			rowsz_;
    int			colsz_;
    int			nrrowblocks_;
    int			nrcolblocks_;

    Coordinates*	coords_;
    Coord3		c00_;
    Coord3		c01_;
    Coord3		c10_;
    Coord3		c11_;

    ObjectSet<SoSplitTexture2Part>	splittextures_;
    ObjectSet<SoTextureCoordinate2>	texturecoords_;
    ObjectSet<SoIndexedFaceSet>		facesets_;
};

}; // Namespace


#endif
