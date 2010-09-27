dTect V4.1
OpendTect commands
May 2010
!


# Select attribute set 'Demo attributes'
Comment "----------Applying Attributes on all Tree items------------"

Case Insensitive

Menu "Analysis`Attributes"
Window "Attribute Set*"
Button "New attribute set"
OnError Continue
Button "No"
Combo "Attribute group" "<All>"
Button "Save on OK" off
OnError Stop

End
