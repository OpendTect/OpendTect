#ifndef vissurvpickset_h
#define vissurvpickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispicksetdisplay.h,v 1.3 2002-03-04 14:20:49 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"

class Color;

namespace visBase { class SceneObjectGroup; };
namespace Geometry { class Pos; }

namespace visSurvey
{

class Scene;

/*!\brief


*/

class PickSet : public visBase::VisualObject
{
public:
    		PickSet( Scene & );
    virtual	~PickSet();

    int		nrPicks();
    
    Geometry::Pos	getPick( int ) const;
    int			addPick( const Geometry::Pos& );

    float	getInlSz() const { return inlsz; }
    float	getCrlSz() const { return inlsz; }
    float	getTSz() const { return inlsz; }

    void	setSize( float inl, float crl, float t );

    void	turnOn(bool);
    bool	isOn() const;
    bool	regForSelection( const VisualObject* assoc=0 ) { return false;}

    void	setColor( const Color& );
    void	switchColorMode( bool totable ) {}
    bool	isColorTable() const { return false; }

    ColorTable*		colorTable() { return 0; }
    const ColorTable*	colorTable() const { return 0; }
    void		colorTableChanged() {}

protected:
    visBase::SceneObjectGroup*	group;

    Scene&	scene;
    Color&	color;

    float	inlsz;
    float	crlsz;
    float	tsz;
};

};


#endif
