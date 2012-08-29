/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2008
-*/


bool Math::IsNormalNumber( mTYPE x )
{
    return finite( x );
}


mTYPE Math::IntPowerOf( mTYPE x, int y )
{
    if ( mIsUdf(x) )
	return mUdf(mTYPE);

    if ( x == 0 )
	return y ? (mTYPE)0 : (mTYPE)1;

    if ( x > 1.5 || x < -1.5 )
    {
	if ( y > 150 ) return mUdf(mTYPE);
	if ( y < -150 ) return 0;
	if ( x > 1.99 || x < -1.99 )
	{
	    if ( y > 100 ) return mUdf(mTYPE);
	    if ( y < -100 ) return 0;
	}
    }
    else if ( x < 0.5 && x > -0.5 )
    {
	if ( y > 100 ) return 0;
	if ( y < -100 ) return 1;
    }

    mTYPE ret = 1;
    while ( y )
    {
	if ( y > 0 )
	    { ret *= x; y--; }
	else
	    { ret /= x; y++; }
    }
    return ret;
}
 

mTYPE Math::PowerOf( mTYPE x, mTYPE y )
{
    if ( x == 0 ) return y ? 0 : 1;
 
    const bool isneg = x < 0 ? 1 : 0;
    if ( isneg ) x = -x;
 
    mTYPE ret = exp( y * log(x) );
    return isneg ? -ret : ret;
}


mTYPE Math::ACos( mTYPE c )
{
    return c >= 1 ? 0 : (c <= -1 ? mTYPE(M_PI) : acos( c ));
}


mTYPE Math::ASin( mTYPE s )
{
    return s >= 1 ? mTYPE(M_PI_2) : (s <= -1 ? -mTYPE(M_PI_2) : asin( s ));
}


mTYPE Math::Log( mTYPE s )
{
    return s <= 0 ? mUdf(mTYPE) : log( s );
}


mTYPE Math::Log10( mTYPE s )
{
    return s <= 0 ? mUdf(mTYPE) : log10( s );
}


mTYPE Math::Sqrt( mTYPE s )
{
    return s <= 0 ? 0 : sqrt( s );
}

mTYPE Math::toDB( mTYPE s )
{
    return s <= 0 ? mUdf(mTYPE) : 20*log10( s );
}
