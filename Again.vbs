dim c
dim F

'set c = createobject ("ASCOM.Utilities.Chooser")
'c.DeviceType = "Focuser"
'dim F 
'F = c.Choose ("ASCOM.Simulator.Focuser")

set F = createobject ("EasyFocus.Focuser")

F.SetupDialog()                                   
'F.Connect = true
if (F.TempCompAvailable) then
	wscript.echo "Comp available "
else
	wscript.echo "No Comp" 
end if
