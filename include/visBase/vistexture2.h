#ifndef vistexture2_h
#define vistexture2_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexture2.h,v 1.6 2003-05-28 09:46:40 kristofer Exp $
________________________________________________________________________


-*/

#include "vistexture.h"

class SoTexture2;
class SoGroup;
template <class T> class Array2D;

namespace visBase
{

class VisColorTab;


/*!\brief

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

    unsigned char*	getTexturePtr();
    void		finishEditing();

    SoTexture2*		texture;
    int			x0sz, x1sz;
};

};

#endif

