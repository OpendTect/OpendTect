#ifndef visdragger_h
#define visdragger_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Marc Gerritsen
 Date:		25-06-2003
 RCS:		$Id: visdragger.h,v 1.1 2003-07-08 09:54:26 jeroen Exp $
________________________________________________________________________


-*/

#include <vissceneobj.h>
#include <position.h>

class SoDragger;


namespace visBase
{

/*!\brief


*/


class Dragger : public SceneObject
{
public:
    Dragger( SoDragger* );
    ~Dragger();
   
    static void         startCB( void*, SoDragger*);
    static void         moveCB( void*, SoDragger*);
    static void         stopCB( void*, SoDragger*);
	    
    
    virtual const Coord3	getTranslation() = 0;
    virtual void		setTranslation( const Coord3& ) = 0;

    Notifier<Dragger>		starthappened;
    Notifier<Dragger>		stophappened;
    Notifier<Dragger>		movehappened;
protected:
    SoDragger*			dragger;
};  
   

}; // Namespace


#endif
