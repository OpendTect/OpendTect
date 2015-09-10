<?php

  function tab_expand($text) {
    $tab_stop = 8;
    $res = "";
    for ( $idx=0; $idx<strlen($text); $idx++ ) {
      if ( $text[$idx]=="\t" ) {
	$curpos = strlen($res);
	$nrtabs = (int) ($curpos/$tab_stop);
	$newsize = ($nrtabs+1)*$tab_stop;
	$nrspaces = $newsize-$curpos;
	for ( $idy=$nrspaces-1; $idy>=0; $idy-- ) {
	  $res = $res." ";
	}
      } 
      else if ( $text[$idx]!=="\r" ) {
	$res = $res.$text[$idx];
      }
     
    }

    return $res;
  }

  function tab_collapse( $string, $tabstop ) {
    $changedline = $string;

    $length = strlen( $changedline );
    $firstchar = 0;
    $last = $tabstop-1;

    for ( $idx=$length-1; $idx>=0; $idx-- ) {

      if ( ($idx+1)%$tabstop )
	 continue;

      $nrspaces = 0;
      for ( $idy=0; $idy<$tabstop; $idy++ )
      {
	    if ( $changedline[$idx-$idy]==' ' )
		$nrspaces++;
	    else
		break;
      }

      if ( $nrspaces<2 )
      {
	if ( $nrspaces==0 || ($idx<$length-1 && $changedline[$idx+1]!=="\t") )
	    continue;
      }

      $lastchartocopy = $idx-$nrspaces;
      $copysize = $lastchartocopy + 1;

      $newstring = "";
      if ( $copysize>0 )
	$newstring .= substr( $changedline, 0, $copysize );

      $newstring .= "\t";
      $newstring .= substr( $changedline, $idx+1 );
      $changedline = $newstring;
    }

    return $changedline;
  }

?>
