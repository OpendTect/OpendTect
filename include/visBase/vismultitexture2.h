#ifndef vismultitexture2_h
#define vismultitexture2_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		Dec 2005
 RCS:		$Id: vismultitexture2.h,v 1.4 2006-03-09 17:06:40 cvskris Exp $
________________________________________________________________________


-*/

#include "vismultitexture.h"
#include "rowcol.h"

class SoSwitch;
class SoMultiTexture2;
class SoComplexity;

namespace visBase
{

class MultiTexture2 : public MultiTexture
{
public:
    static MultiTexture2*	create()
    				mCreateDataObj(MultiTexture2);

    bool			turnOn(bool yn);
    bool			isOn() const;
    void			clearAll();
    				/*!<Sets all arrays to zero. Will cause
				    the texture to become white. After clearAll
				    is called, data of any size will be
				    accepted.*/

    void			setTextureTransparency(int, unsigned char);
    unsigned char		getTextureTransparency(int) const;
    void			setOperation(int texture,Operation);
    Operation			getOperation(int texture) const;
    void			setTextureRenderQuality(float);
    float			getTextureRenderQuality() const;
    bool			setData(int texture,int version,
	    				const Array2D<float>*);
    bool			setIndexData(int texture,int version,
					     const Array2D<unsigned char>*);

    SoNode*			getInventorNode();
protected:

    			~MultiTexture2();

    void		updateSoTextureInternal( int texture );
    void		insertTextureInternal( int texture );
    void		removeTextureInternal( int texture );

    void		updateColorTables();

    SoSwitch*		onoff_;
    SoMultiTexture2*	texture_;
    SoComplexity*	complexity_;
    RowCol		size_;
};

}; // Namespace

#endif
