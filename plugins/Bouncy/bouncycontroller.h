#ifndef bouncycontroller_h
#define bouncycontroller_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
 * ID       : $Id: bouncycontroller.h,v 1.4 2009-09-14 22:55:22 cvskarthika Exp $
-*/

#include "position.h"
#include "timer.h"

namespace Bouncy
{

mClass BouncyController : public CallBacker
{
public:

			        BouncyController();
    virtual			~BouncyController();
    
    void			init(Coord3 pos, Coord minpos, Coord maxpos, 
	    			     float zStart, float zStop, 
	    			     bool simulategame=false);
    void			stop();
    void			pause(bool);

    void			setPos(const Coord3&);
    Coord3			getPos() const;

    Notifier<BouncyController>	newPosAvailable;

protected:

    void			simulateCB(CallBacker*);
    void 			runCB(CallBacker*);

    Coord3			currpos_;
    Coord3			newposdelta_;  
    Coord3			targetpos_;
    Coord			minpos_;
    Coord			maxpos_;
    float			zStart_;
    float			zStop_;
    int				numlives_;
    Timer			updatetimer_;
    bool			simulate_;
    int				nummovesrem_;

};

};


#endif
