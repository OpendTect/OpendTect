#ifndef vistexturerect_h
#define vistexturerect_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: vistexturerect.h,v 1.25 2003-05-27 15:26:53 nanne Exp $
________________________________________________________________________


-*/


#include "visobject.h"


template <class T> class Array2D;
class Coord3;

namespace visBase
{
class VisColorTab;
class Rectangle;
class Texture2;

/*!\brief
    A TextureRect is a Rectangle with a datatexture. 
    The data is set via setData.
*/

class TextureRect : public VisualObjectImpl
{
public:
    static TextureRect*		create()
				mCreateDataObj(TextureRect);

    NotifierAccess*		manipStarts() 	{ return &manipstartnotifier; }
    NotifierAccess*		manipChanges() 	{ return &manipchnotifier; }
    NotifierAccess*		manipEnds() 	{ return &manipendsnotifier; }

    void			setTexture(Texture2&,int idx=0);
    Texture2&			getTexture(int idx=0);

    void			useTexture(bool);
    bool			usesTexture() const;

    void			setRectangle(Rectangle*);
    const Rectangle&		getRectangle() const;
    Rectangle&			getRectangle();

    void			setColorTab(VisColorTab&);
    const VisColorTab&		getColorTab() const;
    VisColorTab&		getColorTab();

    void			setAutoScale(bool);
    bool			autoScale() const;

    void			setClipRate(float n);
    				/*!< Should be between 0 and 0.5 */
    float			clipRate() const;

    void			setData(const Array2D<float>&,int,
	    				int colorsel=-1);
    void			showTexture(int);
    void			clear();

    void			setTextureQuality(float);
				/*!< 0 - bad; 1=best */
    float			getTextureQuality() const;
    void			setResolution(int);
    int				getNrResolutions() const;
    int				getResolution() const;

    const TypeSet<float>&       getHistogram() const;

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

    static const char*		rectangleidstr;
    static const char*		textureidstr;

protected:

    void			triggerManipStarts() 
    				{ manipstartnotifier.trigger(); }
    void			triggerManipChanges() 
    				{ manipchnotifier.trigger(); }
    void			triggerManipEnds() 
    				{ manipendsnotifier.trigger(); }

    SoSwitch*			textureswitch;
    ObjectSet<Texture2> 	textureset;
    Rectangle*			rectangle;
    int				curidx;

    int				useOldPar( const IOPar& );

private:
				~TextureRect();

    Notifier<TextureRect>	manipstartnotifier;
    Notifier<TextureRect>	manipchnotifier;
    Notifier<TextureRect>	manipendsnotifier;
};

};

#endif
