#ifndef vistexture3_h
#define vistexture3_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexture3.h,v 1.3 2003-01-28 07:59:44 kristofer Exp $
________________________________________________________________________


-*/

#include "vistexture.h"

class SoTexture3;
class SoGroup;
template <class T> class Array3D;

namespace visBase
{

/*!\brief

*/

class Texture3 : public Texture
{
public:
    static Texture3*	create()
			mCreateDataObj( Texture3 );

    void		setTextureSize(int, int, int );

    void		setData( const Array3D<float>* );

protected:
    			~Texture3();

    unsigned char*	getTexturePtr();
    void		finishEditing();

    SoTexture3*		texture;
    int			x0sz, x1sz, x2sz;
};

};

#endif
