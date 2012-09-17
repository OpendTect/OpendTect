#ifndef odqtobjset_h
#define odqtobjset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2008
 RCS:		$Id: odqtobjset.h,v 1.3 2009/07/22 16:01:20 cvsbert Exp $
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

    OD*			getODObject(const QT&);
    QT*			getQtObject(const OD&);

protected:

    ObjectSet<OD>	odobjs_;
    ObjectSet<QT>	qtobjs_;
};


template<class OD,class QT>
void ODQtObjectSet<OD,QT>::add( OD* odobj, QT* qtobj )
{
    odobjs_ += odobj;
    qtobjs_ += qtobj;
}

template<class OD,class QT>
void ODQtObjectSet<OD,QT>::remove( const OD& obj )
{
    const int idx = odobjs_.indexOf( &obj );
    if ( idx<0 ) return;

    odobjs_.remove( idx );
    qtobjs_.remove( idx );
}

template<class OD,class QT>
OD* ODQtObjectSet<OD,QT>::getODObject( const QT& qtobj )
{
    const int idx = qtobjs_.indexOf( &qtobj );
    return idx<0 ? 0 : odobjs_[idx];
}

template<class OD,class QT>
QT* ODQtObjectSet<OD,QT>::getQtObject( const OD& obj )
{
    const int idx = odobjs_.indexOf( &obj );
    return idx<0 ? 0 : qtobjs_[idx];
}


#endif
