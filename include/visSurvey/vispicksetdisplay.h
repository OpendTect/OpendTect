#ifndef vissurvpickset_h
#define vissurvpickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispicksetdisplay.h,v 1.1 2002-02-27 14:42:09 kristofer Exp $
________________________________________________________________________


-*/

class BinIDValue;
class Color;

namespace visBase { class SceneObjectGroup; };

namespace visSurvey
{

class Scene;

/*!\brief


*/
class PickSet
{
public:
    		PickSet( Scene & );
    virtual	~PickSet();

    int		nrPicks();
    
    BinIDValue	getPick( int ) const;
    int		addPick( const BinIDValue& );

    float	getInlSz() const { return inlsz; }
    float	getCrlSz() const { return inlsz; }
    float	getTSz() const { return inlsz; }

    void	setSize( float inl, float crl, float t );
    void	setColor( const Color& );
protected:
    visBase::SceneObjectGroup*	group;
    int				groupid;

    Scene&	scene;
    Color&	color;

    float	inlsz;
    float	crlsz;
    float	tsz;
};

};


#endif
