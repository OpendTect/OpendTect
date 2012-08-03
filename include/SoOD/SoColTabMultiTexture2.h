#ifndef SoColTabMultiTexture2_h
#define SoColTabMultiTexture2_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Dec 2005
 RCS:		$Id: SoColTabMultiTexture2.h,v 1.8 2012-08-03 13:00:40 cvskris Exp $
________________________________________________________________________


-*/

#include "soodmod.h"
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoMFBool.h>
#include <Inventor/fields/SoSFImage.h>
#include <Inventor/fields/SoMFEnum.h>
#include <Inventor/fields/SoMFShort.h>
#include <Inventor/SbTime.h>

#include "SoMFImage.h"
#include "soodbasic.h"


class SoFieldSensor;
class SoSensor;
class SbMutex;
class SoGLImage;

/*! SoColTabMultiTexture2 combines one or more textures of the same size into one.
All textures are put in a sequence in SoColTabMultiTexture2::image, and the sequence
is processed from the start to the end to create one texture. */

mClass(SoOD) SoColTabMultiTexture2 : public SoNode
{
    SO_NODE_HEADER( SoColTabMultiTexture2 );
public:

    static void		initClass();
    			SoColTabMultiTexture2();

    static int		getMaxSize();

    enum Operator	{ BLEND, ADD, REPLACE };
   			/*!\enum Operator
			 	 Specifies how a texture should interact
			         with the previous texture(s).
			   \var  Operator BLEND
			   	 The current texture is mixed with the
				 previous texture(s) depending on the
				 value of the opacity component.
			   \var  Operator ADD
			   	 The average between the previous texture(s) and
				 the current texture is taken.
			   \var  Operator REPLACE
			   	 The old texture(s) is replaced by the current
				 one.  */
    enum Component	{ RED=1, GREEN=2, BLUE=4, OPACITY=8 };
    			/*!\enum Component
			 	 Specifies which components that are affected
				 by the current texture. */

    SoMFImagei32	image;
    			/*!< The images themselves. */
    SoMFShort		numcolor;
    			/*!< Number of colors in colortable for each image.
			     If nonzero, the corresponding image must be 1 byte
			     per pixel and map to the colortable.  */
    SoMFShort		opacity;
    			/*!< Specifies the maximum opacity of each image.
			     0 is completely transperant, 255 is completely
			     opaque. */
    SoSFImage		colors;
    			/*!< Colortables for all images. The colortable
			     for image N starts at index=numcolor[0] +
			     numcolor[1] + numcolor[2] + ... +
			     numcolor[N-2] + numcolor[N-1] . */
			     
    SoMFEnum		operation;
    			/*!< Operation for each image. */
    SoMFShort		component;
    			/*!< Component for each image. */

    SoMFBool		enabled;

    SoSFColor		blendColor;
    			/*!<See SoTexture2 for documentation. */

    SoSFEnum		wrapS;	/*!<See SoTexture2 for documentation. */
    SoSFEnum		wrapT;	/*!<See SoTexture2 for documentation. */
    SoSFEnum		model;	/*!<See SoTexture2 for documentation. */

    void		GLRender( SoGLRenderAction* );
    void		callback( SoCallbackAction* );
    void		rayPick( SoRayPickAction* );

    void		setNrThreads( int );
    			/*!<\note Must be called before first GLRender. */

protected:
    void		doAction( SoAction* );
    			~SoColTabMultiTexture2();
    const unsigned char* createImage( SbVec2s&, int & );
    static void		imageChangeCB( void*, SoSensor* );
    static bool		findTransperancy( const unsigned char* colors, int ncol,
	    				  int nc, const unsigned char* idxs=0,
					  int nidx=0 );
    			/*!<Checks if any of the colors in \a colors has
			    a transparency. If idxs and nidx are set,
			    only the colors that are indexed by idxs are
			    checked. */

    SoFieldSensor*	imagesensor_;
    SoFieldSensor*	numcolorsensor_;
    SoFieldSensor*	colorssensor_;
    SoFieldSensor*	operationsensor_;
    SoFieldSensor*	componentsensor_;
    SoFieldSensor*	enabledsensor_;
    SoFieldSensor*	opacitysensor_;

    SbMutex*		glimagemutex_;
    bool		glimagevalid_;
    SoGLImage*		glimage_;
    const unsigned char* imagedata_;
    SbVec2s		imagesize_;
    int			imagenc_;

    int			nrthreads_;
};



#endif
    

