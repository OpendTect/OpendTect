#ifndef vistexturerect_h
#define vistexturerect_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: vistexturerect.h,v 1.2 2002-03-14 14:44:34 nanne Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "geompos.h"


template <class T> class Array2D;
class DataClipper;

class SoTexture2;
class SoSwitch;
class SoComplexity;
class SoGroup;
class SoMaterial;


namespace visBase
{
class VisColorTab;
class Rectangle;

/*!\brief
    A TextureRect is a Rectangle with a datatexture. The data is set via
    setData.
*/

class TextureRect : public VisualObjectImpl
{
public:
    static TextureRect*	create( bool usermanip )
			mCreateDataObj1arg( TextureRect, usermanip );

    i_Notifier*		manipStarts() { return &manipstartnotifier; }
    i_Notifier*		manipChanges() { return &manipchnotifier; }
    i_Notifier*		manipEnds() { return &manipendsnotifier; }

    i_Notifier*         selection() { return &selnotifier; }
    i_Notifier*         deSelection() { return &deselnotifier; }

    void		setRectangle( Rectangle* );
    const Rectangle&	getRectangle() const;
    Rectangle&		getRectangle();

    void		setColorTab( VisColorTab* );
    const VisColorTab&	getColorTab() const;
    VisColorTab&	getColorTab();

    void		setAutoscale(bool n) {autoscale = n; clipData(); }
    bool		autoScale() const { return autoscale; }

    void		setClipRate( float n );
    			/*!< Should be between 0 and 0.5 */
    float		clipRate() const;

    void		setData( const Array2D<float>& );
    void		setTextureQuality(float);
			/*!< 0 - bad; 1=best */

    void		useTexture(bool);
    bool		usesTexture() const;

protected:
    void		updateTexture();
    void		clipData();

    void		triggerSel() { selnotifier.trigger(); }
    void		triggerDeSel() { deselnotifier.trigger(); }
    void		triggerManipStarts() { manipstartnotifier.trigger(); }
    void		triggerManipChanges() { manipchnotifier.trigger(); }
    void		triggerManipEnds() { manipendsnotifier.trigger(); }

    Rectangle*		rectangle;
    VisColorTab*	colortable;

    SoTexture2*		texture;
    SoSwitch*		textureswitch;
    SoComplexity*	quality;
    SoGroup*		texturegrp;
    SoMaterial*		modcolor;

    bool		autoscale;
    float		cliprate;
    Array2D<float>*	data;
private:
			TextureRect( bool usermanip );
			~TextureRect();

    Notifier<TextureRect>	selnotifier;
    Notifier<TextureRect>	deselnotifier;
    Notifier<TextureRect>	manipstartnotifier;
    Notifier<TextureRect>	manipchnotifier;
    Notifier<TextureRect>	manipendsnotifier;
};

};

#endif
