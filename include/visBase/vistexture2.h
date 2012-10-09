#ifndef vistexture2_h
#define vistexture2_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
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

mClass Texture2 : public Texture
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



/*!\brief Set of 2D Textures
Class for managing a set of 2D textures (Texture2). All textures are
added to a SoSwitch node, which means that only one child (a Texture2 node) 
will be visited during rendering.
To let the children share the same properties by default, use the 
share## functions.
*/

mClass Texture2Set : public DataObject
{
public:
    static Texture2Set*	create()
			mCreateDataObj( Texture2Set );

    void		addTexture(Texture2*);
    void		removeTexture(Texture2*);
    void		removeTexture(int);
    void		removeAll(bool keepfirst); 
    			/*!< First texture will not be removed when keepfirst 
    				is true. */
    int			nrTextures() const;

    Texture2*		getTexture(int) const;
    void		setActiveTexture(int); 
    Texture2*		activeTexture() const;
    int			activeTextureNr() const;
    void		finishTextures(); //!< For post texturecreating calls

    void		shareResolution(bool yn)	{ shareres = yn; }
    void		shareColorSequence(bool yn)	{ sharecolseq = yn; }
    bool		resolutionShared() const	{ return shareres; }
    bool		colorSequenceShared() const	{ return sharecolseq; }

protected:
    			~Texture2Set();

    SoSwitch*		textureswitch;
    ObjectSet<Texture2>	textureset;

    bool		shareres; 	//!< default is true;
    bool		sharecolseq;	//!< default is true;

    void		colTabChanged(CallBacker*);

    virtual SoNode*	gtInvntrNode();

};

}

#endif
