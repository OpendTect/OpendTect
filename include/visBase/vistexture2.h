#ifndef vistexture2_h
#define vistexture2_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexture2.h,v 1.5 2003-05-27 15:26:53 nanne Exp $
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
    void		setColorData(const Array2D<float>*,int sel);
    			/*!< sets second dataset that will be used for setting
    			     the transparency,whiteness,brightness or color
			     (specified by sel). */
			     

protected:
    			~Texture2();

    unsigned char*	getTexturePtr();
    void		finishEditing();

    SoTexture2*		texture;
    int			x0sz, x1sz;
};

};

#endif

