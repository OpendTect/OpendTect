#ifndef vistexturerect_h
#define vistexturerect_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: vistexturerect.h,v 1.15 2002-10-14 14:25:26 niclas Exp $
________________________________________________________________________


-*/


#include "visobject.h"


template <class T> class Array2D;

class Coord3;
class DataClipper;

class SoTexture2;
class SoImage;
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
    static TextureRect*	create()
			mCreateDataObj0arg( TextureRect );

    float		getValue( const Coord3& ) const;

    NotifierAccess*	manipStarts() { return &manipstartnotifier; }
    NotifierAccess*	manipChanges() { return &manipchnotifier; }
    NotifierAccess*	manipEnds() { return &manipendsnotifier; }

    void		setRectangle( Rectangle* );
    const Rectangle&	getRectangle() const;
    Rectangle&		getRectangle();

    void		setColorTab( VisColorTab* );
    const VisColorTab&	getColorTab() const;
    VisColorTab&	getColorTab();

    void		setAutoscale(bool n) 	{ autoscale = n; }
    bool		autoScale() const 	{ return autoscale; }

    void		setClipRate( float n );
    			/*!< Should be between 0 and 0.5 */
    float		clipRate() const;

    void		setData( const Array2D<float>& );
    Array2D<float>*	get2DData()	{ return data; }
    void		setTextureQuality(float);
			/*!< 0 - bad; 1=best */
    void		setResolution(int);
    int			getNrResolutions() const;
    int			getResolution() const	{ return resolution; }

    void		useTexture(bool);
    bool		usesTexture() const;

    virtual void	fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int		usePar( const IOPar& );


    static const char*	texturequalitystr;
    static const char*	rectangleidstr;
    static const char*	colortabidstr;
    static const char*	usestexturestr;
    static const char*	clipratestr;
    static const char*	autoscalestr;
    static const char*	resolutionstr;

protected:
    void		updateTexture(CallBacker*);
    void		clipData();

    void		triggerManipStarts() { manipstartnotifier.trigger(); }
    void		triggerManipChanges() { manipchnotifier.trigger(); }
    void		triggerManipEnds() { manipendsnotifier.trigger(); }

    Rectangle*		rectangle;
    VisColorTab*	colortable;

    SoTexture2*		texture;
    SoSwitch*		textureswitch;
    SoComplexity*	quality;
    SoGroup*		texturegrp;

    bool		autoscale;
    float		cliprate;
    Array2D<float>*	data;
    int			resolution;

private:
			~TextureRect();

    Notifier<TextureRect>	manipstartnotifier;
    Notifier<TextureRect>	manipchnotifier;
    Notifier<TextureRect>	manipendsnotifier;
};

};

#endif
