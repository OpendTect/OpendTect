#ifndef vistexture2_h
#define vistexture2_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexture2.h,v 1.1 2003-01-03 11:19:23 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"

class SoTexture2;
class SoGroup;
template <class T> class Array2D;
class DataClipper;

namespace visBase
{

class VisColorTab;


/*!\brief

*/

class Texture2 : public SceneObject
{
public:
    static Texture2*	create()
			mCreateDataObj( Texture2 );

    void		setTextureSize(int, int );

    void		setAutoScale( bool yn );
    bool		autoScale() const;

    void		setColorTab( VisColorTab& );
    VisColorTab&	getColorTab();

    void		setClipRate( float );
    float		clipRate() const;

    void		setData( const Array2D<float>* );

    SoNode*		getData();

protected:
    			~Texture2();
    void		colorTabChCB( CallBacker* );
    void		colorSeqChCB( CallBacker* );

    void		reClipData();
    void		reMapData();
    void		updateTexture();

    SoTexture2*		texture;
    SoGroup*		root;

    Array2D<float>*		cachedata;
    Array2D<unsigned char>*	cacheddataindexes;
    DataClipper&		dataclipper;

    int			x0sz, x1sz;

    bool		autoscale;

    VisColorTab*	colortab;
};

};

#endif

