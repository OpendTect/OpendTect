#ifndef vistexture2_h
#define vistexture2_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexture2.h,v 1.7 2003-10-27 15:57:47 nanne Exp $
________________________________________________________________________


-*/

#include "vistexture.h"

class SoTexture2;
class SoGroup;
template <class T> class Array2D;
class Array2DInfoImpl;

namespace visBase
{

class VisColorTab;

/*!\brief
Used for creating a 2D texture
*/

class Texture2 : public Texture
{
public:
    static Texture2*	create()
			mCreateDataObj( Texture2 );

    void		setTextureSize(int, int );

    void		setData(const Array2D<float>*,DataType sel=Color);
    			/*!< Sets data to texture.
			\param sel=Color	Sets color on texture
			\param sel=Transperancy Sets transperencydata,
						colortable transperancy will
						be overridden.
			\param sel=Hue		The hue of the colortable is
						multiplied with the data.
			\param sel=Saturation	The saturation of the
						colortable is multiplied with
						the data.
			\param sel=Brightness	The brightness of the
						colortable is multiplied with
						the data.
			*/

protected:
    			~Texture2();

    bool		isDataClassified(const Array2D<float>*) const;
    void		polyInterp(const Array2DInfoImpl&,
	    			   const Array2D<float>*,float*);
    void		nearestValInterp(const Array2DInfoImpl&,
					 const Array2D<float>*,float*);

    unsigned char*	getTexturePtr();
    void		finishEditing();

    SoTexture2*		texture;
    int			x0sz, x1sz;
};

};

#endif

