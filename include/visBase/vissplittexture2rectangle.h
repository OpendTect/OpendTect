#ifndef vissplittexture2rectangle_h
#define vissplittexture2rectangle_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		2-28-2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "position.h"
#include "visobject.h"

class SoSeparator;
class SoIndexedFaceSet;
class SoTextureCoordinate2;
class SoTextureComposer;

namespace visBase
{

class Coordinates;
/*!Rectangle shape that is divided into smaller parts, where each part
   has its own (small) texture, derived from a MultiTexture2.
   Object must be notified of MultiTexture2's size, splitting status and 
   used texture units. */

mClass SplitTexture2Rectangle : public VisualObjectImpl
{
public:
    static SplitTexture2Rectangle* create()
			mCreateDataObj(SplitTexture2Rectangle);

    void		setOriginalTextureSize(int sz0,int sz1);

    void		setPosition(const Coord3& c00,const Coord3& c01,
				    const Coord3& c10,const Coord3& c11);
    const Coord3&	getPosition(bool dim0,bool dim1) const;
    
    const mVisTrans* 	getDisplayTransformation() const;
    void		setDisplayTransformation(const mVisTrans*);

protected:
    			~SplitTexture2Rectangle();
    void		updateFaceSets(); 
    void		updateCoordinates(); 
    void		updateSeparator(SoSeparator*, SoIndexedFaceSet*&,
			    SoTextureCoordinate2*&,SoTextureComposer*&) const;
    void		updateSeparator(SoSeparator*, SoIndexedFaceSet*&) const;

    TypeSet<float>	c00factors_;
    TypeSet<float>	c01factors_;
    TypeSet<float>	c10factors_;
    TypeSet<float>	c11factors_;

    int			rowsz_;
    int			colsz_;
    int			nrrowblocks_;
    int			nrcolblocks_;

    Coord3		c00_;
    Coord3		c01_;
    Coord3		c10_;
    Coord3		c11_;

    Coordinates*		coords_;
    ObjectSet<SoSeparator>	separators_;
};

}; // Namespace


#endif
