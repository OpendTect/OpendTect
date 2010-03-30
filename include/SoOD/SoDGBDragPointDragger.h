#ifndef SoDGBDragPointDragger_h
#define SoDGBDragPointDragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          March 2010
 RCS:           $Id: SoDGBDragPointDragger.h,v 1.1 2010-03-30 08:04:18 cvskarthika Exp $
________________________________________________________________________


-*/

#include <Inventor/draggers/SoDragPointDragger.h>

#include "soodbasic.h"


/*!\brief
This class is basically a SoDragPointDragger, which overcomes the undesirable 
effects of the SoDragPointDragger when it is very small. See src file for more 
details.
*/

mClass SoDGBDragPointDragger : public SoDragPointDragger
{

public:
    static void		initClass();
    			SoDGBDragPointDragger();

protected:
	void		dragStart(void);
	static void startCB(void * f, SoDragger * d);
    
private:

};

#endif

