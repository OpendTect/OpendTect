#ifndef vistexture2_h
#define vistexture2_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexture2.h,v 1.3 2003-01-23 11:58:23 nanne Exp $
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

    void		setData( const Array2D<float>* );

    SoNode*		getData();

protected:
    			~Texture2();

    unsigned char*	getTexturePtr();
    void		finishEditing();

    SoTexture2*		texture;
    SoGroup*		root;

    int			x0sz, x1sz;
};

};

#endif

