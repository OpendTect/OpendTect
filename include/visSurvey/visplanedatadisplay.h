#ifndef visplanedatadisplay_h
#define visplanedatadisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visplanedatadisplay.h,v 1.1 2002-03-21 10:22:38 bert Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "attribsel.h"

namespace Geometry { class Pos; }
namespace visBase { class TextureRect; };

namespace visSurvey
{
/*!\brief
*/

class PlaneDataDisplay : public visBase::VisualObject
{
public:

    enum Type		{ Inline, Crossline, Timeslice };

    static PlaneDataDisplay* create( Type type )
			mCreateDataObj1arg( PlaneDataDisplay, Type, type );

    Type		getType() const { return type; }

    AttribSelSpec	getAttribId() const { return attrsel; }
    void		setAttribId( AttribSelSpec newattrsel )
    			{ attrsel = newattrsel; }

    int			getNewTextureData(bool manippos=false);
    			/*!< 0 - Cancel
			     -1 - Other error
			     1 - Success
			*/

    void		turnOn(bool);
    bool		isOn() const;

    void			setMaterial( visBase::Material* );
    const visBase::Material*	getMaterial() const;
    visBase::Material*		getMaterial();

    const visBase::TextureRect&	textureRect() const { return *trect; }
    visBase::TextureRect&	textureRect() { return *trect; }

    SoNode*			getData();

    NotifierAccess*		selection();
    NotifierAccess*		deSelection();
protected:
				~PlaneDataDisplay();

    void			select();
    void			deSelect();

    visBase::TextureRect*	trect;

    Type			type;
    bool			selected;

    AttribSelSpec		attrsel;
};

};


#endif
