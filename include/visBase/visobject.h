#ifndef visobject_h
#define visobject_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visobject.h,v 1.13 2002-03-18 14:45:35 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"
#include "color.h"

class SoSeparator;
class SoNode;
class SoSwitch;
class SoDrawStyle;

namespace visBase
{
class Material;

/*!\brief
    Base class for all objects that are visual on the scene.
*/

class VisualObject : public SceneObject
{
public:
    virtual void	turnOn(bool)					= 0;
    virtual bool	isOn() const					= 0;

    virtual void		setMaterial( Material* )		= 0;
    virtual const Material*	getMaterial() const			= 0;
    virtual Material*		getMaterial()				= 0;

    virtual int		usePar( const IOPar& iopar )
			{ return SceneObject::usePar(iopar); }
    virtual void	fillPar( IOPar& iopar ) const
			{ SceneObject::fillPar( iopar );}
};


class VisualObjectImpl : public VisualObject
{
public:
    void		turnOn(bool);
    bool		isOn() const;

    void		setMaterial( Material* );
    const Material*	getMaterial() const { return material; }
    Material*		getMaterial() { return material; }

    SoNode*		getData();

    virtual int		usePar( const IOPar& iopar );
    virtual void	fillPar( IOPar& iopar ) const;
protected:
    void		addChild( SoNode* );
    void		insertChild( int pos, SoNode* );
    void		removeChild( SoNode* );

			VisualObjectImpl();
    virtual		~VisualObjectImpl();

    SoSwitch*		onoff;
    SoDrawStyle*	drawstyle;

    Material*		material;

private:
    SoSeparator*	root;

    static const char*	materialidstr;
    static const char*	isonstr;
};

};


#endif
