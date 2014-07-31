#ifndef uisegydirectinserter_h
#define uisegydirectinserter_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiioobjinserter.h"

class uiSEGYRead;


mExpClass(uiSeis) uiSEGYDirectVolInserter : public uiIOObjInserter
{
public:

				uiSEGYDirectVolInserter();

    virtual uiToolButtonSetup*	getButtonSetup() const;
    static uiIOObjInserter*	create()
				{ return new uiSEGYDirectVolInserter; }

    static void			initClass();

protected:

    uiSEGYRead*		segyread_;

    void		startScan(CallBacker*);
    void		scanComplete(CallBacker*);

};


#endif
