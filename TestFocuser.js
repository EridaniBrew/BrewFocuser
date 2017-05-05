
var c = new ActiveXObject("ASCOM.Utilities.Chooser");    // Change for your driver's ID
c.DeviceType = "Focuser";
var F = c.Choose();
//WScript.StdOut.WriteLine(name.);

//var F = new ActiveXObject(name); 
/****
F.SetupDialog();                                    // Comment this out once you set COM port, etc.
F.Connected = true;
if (F.TempCompAvailable)
	{
	WScript.StdOut.WriteLine("Comp available ");
	} else{
	WScript.StdOut.WriteLine("No Comp" );
	}
WScript.StdOut.Write("Press Enter to quit and release the driver ");
WScript.StdIn.ReadLine();
***/