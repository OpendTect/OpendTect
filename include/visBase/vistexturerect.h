#ifndef vistexturerect_h
#define vistexturerect_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: vistexturerect.h,v 1.35 2011-02-10 05:11:27 cvssatyaki Exp $
________________________________________________________________________


-*/


#include "visobject.h"


template <class T> class Array2D;
template <class T> class Interval;
class Coord3;

namespace visBase
{
class VisColorTab;
class Rectangle;
class Texture2Set;

/*!\brief
    A TextureRect is a Rectangle with a datatexture. 
    The data is set via setData.
*/

mClass TextureRect : public VisualObjectImpl
{
public:
    static TextureRect*		create()
				mCreateDataObj(TextureRect);

    NotifierAccess*		manipStarts() 	{ return &manipstartnotifier; }
    NotifierAccess*		manipChanges() 	{ return &manipchnotifier; }
    NotifierAccess*		manipEnds() 	{ return &manipendsnotifier; }

    void			addTexture();
    void			useTexture(bool);
    bool			usesTexture() const;
    void			showTexture(int);
    int				shownTexture() const;
    void			removeAllTextures(bool);

    void			setRectangle(Rectangle*);
    const Rectangle&		getRectangle() const;
    Rectangle&			getRectangle();

    void			setColorTab(VisColorTab&);
    const VisColorTab&		getColorTab() const;
    VisColorTab&		getColorTab();

    void			setAutoScale(bool);
    bool			autoScale() const;

    void			setClipRate(Interval<float>);
    				/*!< Should be between 0 and 0.5 */
    Interval<float>		clipRate() const;

    void			setData(const Array2D<float>*,int idx=0,
	    				int colorsel=0);

    void			setTextureQuality(float);
				/*!< 0 - bad; 1=best */
    float			getTextureQuality() const;
    void			setResolution(int);
    int				getNrResolutions() const;
    int				getResolution() const;
    void			finishTextures();

    const TypeSet<float>&       getHistogram() const;

    void			setColorPars(bool,bool,const Interval<float>&);
    const Interval<float>&	getColorDataRange() const;

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

    static const char*		rectangleidstr();
    static const char*		textureidstr();

protected:

    void			triggerManipStarts() 
    				{ manipstartnotifier.trigger(); }
    void			triggerManipChanges() 
    				{ manipchnotifier.trigger(); }
    void			triggerManipEnds() 
    				{ manipendsnotifier.trigger(); }

    Texture2Set*	 	textureset;
    Rectangle*			rectangle;

private:
				~TextureRect();

    Notifier<TextureRect>	manipstartnotifier;
    Notifier<TextureRect>	manipchnotifier;
    Notifier<TextureRect>	manipendsnotifier;
};

};

#endif
