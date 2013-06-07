#ifndef visswitch_h
#define visswitch_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		Sep 2012
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visdata.h"

class SoSeparator;
class SoSwitch;

namespace visBase
{

/*!SoSeparator and SoSwitch from Coin. */

mClass Switch : public DataObject
{
public:

    static Switch*		create() mCreateDataObj(Switch);

    void			turnOn(bool yn);
    void			addChild(SoNode*);
    void			removeChild(SoNode*);

protected:
    				~Switch();
    SoSwitch*			switch_;
    virtual SoNode*		gtInvntrNode();

public:
    bool			isOn() const;
};

mClass Separator : public DataObject
{
public:

    static Separator*		create() mCreateDataObj(Separator);

    void			addChild(SoNode*);
    void			removeChild(SoNode*);

protected:
    				~Separator();
    SoSeparator*		separator_;
    virtual SoNode*		gtInvntrNode();
};


}

#endif

