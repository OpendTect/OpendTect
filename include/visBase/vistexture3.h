#ifndef vistexture3_h
#define vistexture3_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexture3.h,v 1.1 2003-01-08 14:15:48 kristofer Exp $
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

    SoNode*		getData();

protected:
    			~Texture3();
    void		setTexture(const unsigned char*);

    SoTexture3*		texture;
    SoGroup*		root;

    int			x0sz, x1sz, x2sz;
};

};

#endif
