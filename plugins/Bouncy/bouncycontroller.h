#ifndef bouncycontroller_h
#define bouncycontroller_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
 * ID       : $Id: bouncycontroller.h,v 1.1 2009-09-08 08:44:31 cvskarthika Exp $
-*/

#include "executor.h"
#include "position.h"

namespace Bouncy
{

mClass BouncyController : public Executor
{
public:

				BouncyController(const char*);
    virtual			~BouncyController();
    
    void			init(Coord3);
    int				nextStep();
    void			stop();

    Coord3			findNewPos(Coord3);

    Notifier<BouncyController>	newPosAvailable;

protected:
    
    Coord3			currpos_;
    Coord3			newposdelta_;  
        // to do: check if it needs to be a queue

    Coord3			targetpos_;
    int				numlives_;

};

};


#endif
