#ifndef odqtobjset_h
#define odqtobjset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		April 2008
 RCS:		$Id: odqtobjset.h,v 1.1 2008-06-27 11:20:01 cvsnanne Exp $
________________________________________________________________________

-*/


#include "sets.h"


template<class OD,class QT>
class ODQtObjectSet
{
public:
    			ODQtObjectSet()		{}

    void		add(OD*,QT*);
    void		remove(const OD&);

    OD*			getODObject(const QT&)
    QT*			getQtObject(const OD&);

protected:

    ObjectSet<OD>	odobjs_;
    ObjectSet<QT>	qtobjs_;
};


template<class OD,class QT>
void ODQtObjectSet::add( OD* odobj, QT* qtobj )
{
    odobjs_ += odobj;
    qtobjs_ += qtobj;
}

template<class OD,class QT>
void ODQtObjectSet::remove( const OD& obj )
{
    const int idx = odobjs_.indexOf( obj );
    if ( idx<0 ) return;

    odobjs_.remove( idx );
    qtobjs_.remove( idx );
}

template<class OD,class QT>
OD* ODQtObjectSet::getODObject( const QT& qtobj )
{
    const int idx = qtobjs_.indexOf( obj );
    return idx<0 ? 0 : odobjs_[idx];
}

template<class OD,class QT>
QT* ODQtObjectSet::getQtObject( const OD& obj )
{
    const int idx = odobjs_.indexOf( obj );
    return idx<0 ? 0 : qtobjs_[idx];
}


#endif
