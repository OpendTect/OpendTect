#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "sets.h"


template<class OD,class QT>
mClass(uiBase) ODQtObjectSet
{
public:
    			ODQtObjectSet()		{}

   int			size() const		{ return odobjs_.size(); }

    void		add(OD*,QT*);
    void		remove(const OD&);

    OD*			getODObject(const QT&);
    QT*			getQtObject(const OD&);

    OD*			getODObject(int idx);
    QT*			getQtObject(int idx);

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

    odobjs_.removeSingle( idx );
    qtobjs_.removeSingle( idx );
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


template<class OD,class QT>
OD* ODQtObjectSet<OD,QT>::getODObject( int idx )
{
    return odobjs_.validIdx(idx) ? odobjs_[idx] : 0;
}


template<class OD,class QT>
QT* ODQtObjectSet<OD,QT>::getQtObject( int idx )
{
    return qtobjs_.validIdx(idx) ? qtobjs_[idx] : 0;
}
