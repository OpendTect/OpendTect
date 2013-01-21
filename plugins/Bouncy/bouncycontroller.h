#ifndef bouncycontroller_h
#define bouncycontroller_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
 * ID       : $Id$
-*/

#include "bouncymod.h"
#include "position.h"
#include "timer.h"

namespace Bouncy
{

mExpClass(Bouncy) BouncyController : public CallBacker
{
public:

			        BouncyController();
    virtual			~BouncyController();
    
    void			init(const Coord3& pos, const Coord& minpos, 
	    			     const Coord& maxpos, 
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

