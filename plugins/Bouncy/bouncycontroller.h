#ifndef bouncycontroller_h
#define bouncycontroller_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
 * ID       : $Id: bouncycontroller.h,v 1.2 2009-09-09 11:44:37 cvskarthika Exp $
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

    void			setPos(const Coord3&);
    Coord3			getPos() const;

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
