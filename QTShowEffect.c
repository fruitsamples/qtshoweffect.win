//////////
//
//	File:		QTShowEffect.c
//
//	Contains:	Code to generate a QuickTime movie with a QuickTime video effect in it,
//				and to preview an effect on zero, one, or two pictures.
//
//	Written by:	Tim Monroe
//				Based on existing ShowEffect code written by Dan Crow,
//				which was adapted from Peter Hoddie's original DoEffect code.
//
//	Copyright:	� 1996-1999 by Apple Computer, Inc., all rights reserved.
//
//	Change History (most recent first):
//
//	   <37>	 	03/19/01	rtm		added MacSetPort call to QTEffects_HandleEffectsDialogEvents (so GlobalToLocal
//									would work correctly)
//	   <36>	 	09/01/00	rtm		made gChooseDialog global, so we can just hide and show it instead of rebuilding
//									it every time; this fixes a problem with RezWacked files on Windows (menu was
//									getting lost...); commented out call to QTEffects_UninitializePopUpMenu
//	   <35>	 	06/08/00	rtm		fixed crashing bug in QTEffects_SetUpEffectSequence caused by illegal cast 
//	   <34>	 	06/07/00	rtm		fixed dynamic building of Effects pop-up menu, using info in Tech Note TB42;
//									added QTEffects_UninitializePopUpMenu
//	   <33>	 	03/20/00	rtm		made changes to get things running under CarbonLib
//	   <32>	 	05/24/99	rtm		fixed problem that caused image buffers to get corrupted on certain operations;
//									we were not careful in our locking and unlocking the image pixmaps, so sometimes
//									they would get unlocked and remain unlocked; now we lock the image pixmaps for
//									gGW1 and gGW2 when the GWorlds are created, and then we leave them locked forever
//	   <31>	 	05/03/99	rtm		made call to GetCCursor cross-platform again; changed name of main effects
//									display window; removed most support for compound effects; converted some
//									hard-coded strings into constants; on Windows, made main effects display window
//									a child of ghWndMDIClient (still some remaining cosmetic glitches, tho')
//	   <30>	 	03/23/99	rtm		started to add support for compound effects: renamed _CreateTwoSourceEffectDescription
//									as _CreateEffectDescription
//	   <29>	 	03/02/99	rtm		added support for MakeImageDescriptionForEffect (QT 4.0 and later)
//	   <28>	 	02/04/99	rtm		made call to GetCCursor Mac-only (crashes on Windows); filed bug report
//	   <27>	 	09/30/98	rtm		tweaked call to AddMovieResource to create single-fork movies
//	   <26>	 	05/21/98	rtm		removed conditionalized support for compound effects
//	   <25>	 	05/20/98	rtm		changed stacked calls to NewGWorld to a single QTNewGWorld call in the
//									functions QTEffects_GetPictResourceAsGWorld and QTEffects_GetPictureAsGWorld;
//									fixed crashing bug in QTEffects_HandleEffectsDialogEvents by adding the test
//									"&& (gSubPanelPopUpControl != NULL)" in control handle test
//	   <24>	 	04/23/98	rtm		changed gw->portPixMap to GetGWorldPixMap in CopyBits calls
//	   <23>	 	03/24/98	rtm		fixed minor problems with subpanels in custom dialog box
//	   <22>	 	03/23/98	rtm		added support for subpanels in custom dialog box
//	   <21>	 	03/09/98	rtm		changed kEffectSourceName constant in QTEffects_AddTrackReferenceToInputMap
//									back to kEffectDataSourceType; now "Build Movie" works again
//	   <20>	 	03/06/98	rtm		moved custom dialog item handling to before the call to
//									ImageCodecIsStandardParameterDialogEvent in QTEffects_HandleEffectsDialogEvents
//	   <19>	 	03/02/98	rtm		further clean-up on Windows
//	   <18>	 	02/26/98	rtm		added QTEffects_HandleEffectsDialogEvents, QTEffects_EffectsDialogCallback,
//									and QTEffects_CustomDialogWndProc to properly handle Windows dialog boxes;
//									miscellaneous other clean-up; added QTUtils_SetMovieFileLoopingInfo to
//									QTEffects_CreateEffectsMovie
//	   <17>	 	02/21/98	rtm		added palindrome looping and stepping backward
//	   <16>	 	02/20/98	rtm		revised custom dialog box handling; now works on Windows (yippee!)
//	   <15>	 	02/18/98	rtm		removed support for thumbnail movies
//	   <14>	 	02/17/98	rtm		conditionalized support for compound effects (not yet implemented by QT 3.0)
//	   <13>	 	02/16/98	rtm		added QTEffects_HandleEffectsWindowEvents and *Messages for platform-specific
//									event/message handling for main effects window
//	   <12>	 	02/13/98	rtm		reworked QTEffects_DrawEffectsWindow to support stepping through an effect
//	   <11>	 	02/12/98	rtm		removed all support for Preferences dialog box (Settings menu is enough)
//	   <10>	 	02/11/98	rtm		reworked QTEffects_CreateTwoSourceEffectDescription to return an effect
//									description instead of setting gCurrentState.fEffectDescription directly;
//									began adding support for compound effects
//	   <9>	 	02/10/98	rtm		added call to WriteResource to QTEffects_AddListOfEffects
//	   <8>	 	02/07/98	rtm		rewrote QTEffects_AddListOfEffects to use QTGetEffectsList
//	   <7>	 	02/04/98	rtm		fixed problems with custom dialog boxes; see the discussion in the
//									function QTEffects_LetUserCustomizeEffect for details
//	   <6>	 	02/03/98	rtm		modified QTEffects_DrawEffectsWindow and QTEffects_AddVideoTrackFromGWorld
//									to copy image(s) from existing GWorld(s); added ability to read pictures
//									from files (in addition to resources) using graphics importer routines
//	   <5>	 	01/31/98	rtm		added Settings menu to replace Preferences dialog box
//	   <4>	 	01/30/98	rtm		fixed video track durations; now everything works fine on MacOS
//	   <3>	 	12/19/97	rtm		added gModalFilterUPP to ModalDialog so that the Select Effect
//									dialog box can be moved
//	   <2>	 	12/18/97	rtm		continued clean-up: removed unused global variables and added
//									constants for dialog box item indices
//	   <1>	 	12/15/97	rtm		first file; integrated existing code with shell framework
//	   
//	This file defines functions that display a QuickTime video effect as a transition from one picture
//	to another, or a QuickTime video effect applied to a single picture. The user selects the effect
//	using a simple dialog box with a pop-up menu; then the user selects effect parameters using either
//	the standard effects parameter dialog box or a custom effects parameter dialog box. Once an effect
//	and some parameters are selected, the user can:
//
//		* run the effect once through from beginning to end
//		* run the effect continuously in a normal or palindrome loop
//		* run a single step of the effect, either forward or backward
//		* generate a QuickTime movie file containing the current effect
//		* select the picture(s) to which the effect is applied
//
//	Here we use the low-level QuickTime video effects APIs, for greater control over the process. The low-level
//	API includes functions beginning "ImageCodec", for example: ImageCodecCreateStandardParameterDialog.
//	The main advantage to using the low-level calls is the ability to embed the effect parameter dialog items
//	into a custom, application-specific dialog box. (We do however use the high-level function QTGetEffectsList
//	when building a list of the available effects.)
//	
//	NOTE:
//	If you set the compiler flag ALLOW_COMPOUND_EFFECTS to 1, then the Build Effect Movie menu item will add
//	the film noise filter to whatever two-source effect you've already chosen when it builds the movie; this is
//	intended to illustrate how to work with compound (or "stacked") effects. QTShowEffect does not yet provide
//	a mechanism to allow the user to add a compound effect to the one shown in the main effects display window.
//
//////////

//////////
//
// TO DO:
// + 
//
//////////

//////////
//
// header files
//
//////////

#include "QTShowEffect.h"


//////////
//
// global variables
//
//////////

WindowPtr					gMainWindow = NULL;				// the main effect-display window
QTParameterDialog			gEffectsDialog = 0L;			// identifier for the standard parameter dialog box
DialogPtr					gCustomDialog = NULL;			// the dialog that incorporates the standard parameter dialog box user interface elements
DialogPtr					gChooseDialog = NULL;			// the dialog for choosing an effect
GWorldPtr					gGW1 = NULL;					// the GWorlds that hold the effect sources
GWorldPtr					gGW2 = NULL;
ImageDescriptionHandle		gGW1Desc = NULL;				// the image descriptions of the GWorlds
ImageDescriptionHandle		gGW2Desc = NULL;
ComponentInstance			gCompInstance = NULL;			// the instance of the current effect component
unsigned short				gLoopingState = kNormalLooping;	// the current looping state of effect display
unsigned short				gCurrentDir = kForward;			// the current direction of effect display
Boolean						gUseStandardDialog = true;		// if true, use the standard effect parameter dialog box; if false, use a custom effect parameter dialog box
Boolean						gFastEffectDisplay = false;		// if true, the effect is run to completion immediately;
															// if false, the effect runs as tickled by the event loop or message stream
PopUpMenuInformation		gSelectEffectPopup;				// holds information about the Select Effect popup menu
StateInformation			gCurrentState;					// holds information about the current state of effects processing
int							gNumberOfSteps = k30StepsCount;
MenuHandle					gSubPanelPopUpMenu = NULL;		// menu handle for subpanel pop-up menu in custom dialog box
ControlHandle				gSubPanelPopUpControl = NULL;	// control handle for subpanel pop-up menu in custom dialog box

extern ModalFilterUPP		gModalFilterUPP;

#if TARGET_OS_WIN32
char						gEffectsWindowClassName[] = kEffectsWindowClassName;
extern HANDLE				ghInst;							// the instance of this application
extern HWND					ghWndMDIClient; 				// the MDI client window
extern Boolean				gShuttingDown;
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Application start-up and shut-down functions.
//
// Use these functions to initialize and tear down effects processing for this application.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////
//
// QTEffects_Init
// Do application-specific initialization for effects processing.
//
//////////

OSErr QTEffects_Init (void)
{
	CCrsrHandle			myCursor = NULL;
	OSErr				myErr = noErr;

#if TARGET_OS_WIN32
	HWND				myWindow;
#endif	

#if TARGET_OS_MAC	
	StringPtr			myString;
	Rect				myRect;
	DialogPtr			myDialog = NULL;

	// show a splash screen while we set up the default values and other such things
	myDialog = GetNewDialog(kSplashDialogID, NULL, (WindowPtr)-1);
	if (myDialog != NULL) {
		MacSetPort(GetDialogPort(myDialog));
		MacShowWindow(GetDialogWindow(myDialog));
		DrawDialog(myDialog);
	}
#endif

	// get wristwatch cursor; this initialization might take a little while....
	myCursor = GetCCursor(kWatchCursorResID);
	if (myCursor != NULL)
		SetCCursor(myCursor);
	
	// set up the initial state
	gCurrentState.fSampleDescription = NULL;
	gCurrentState.fEffectDescription = NULL;
	gCurrentState.fEffectType        = kCrossFadeTransitionType;
	gCurrentState.fShowingEffect     = false;
	gCurrentState.fTime              = 0;
	gCurrentState.fEffectSequenceID  = 0L;
	gCurrentState.fTimeBase          = NULL;
	
	// create the pop-up menu for the Select Effect dialog box
	myErr = QTEffects_InitializePopUpMenu(&gSelectEffectPopup);
	if (myErr != noErr)
		goto bail;
	
	// add items to the menu
	myErr = QTEffects_AddListOfEffects();
	if (myErr != noErr) {
#if TARGET_OS_MAC	
		QTFrame_ShowWarning("\pCannot run this application from a locked volume; please copy to a different disk. Quitting.", 0);
		ExitToShell();
#endif
#if TARGET_OS_WIN32
		PostQuitMessage(0);
#endif
	}
		
	// create the GWorlds containing the pictures that will act as the sources for the effect to be displayed;
	// on application start-up, we'll use canned pictures in resources
	myErr = QTEffects_GetPictResourceAsGWorld(kFirstPICTResID, kWidth, kHeight, kDepth, &gGW1);
	if (myErr != noErr)
		goto bail;
		
	myErr = QTEffects_GetPictResourceAsGWorld(kSecondPICTResID, kWidth, kHeight, kDepth, &gGW2);
	if (myErr != noErr)
		goto bail;
		
	// lock the pixmaps; henceforth, whenever we draw into or copy out of these GWorlds, we shall assume that
	// the pixmaps have previously been locked
	if (!LockPixels(GetGWorldPixMap(gGW1)))
		goto bail;
		
	if (!LockPixels(GetGWorldPixMap(gGW2)))
		goto bail;

	// ***create the main effects display window***
#if TARGET_OS_MAC	
	// on MacOS, we create a standard Macintosh window and keep track of it in the global variable gMainWindow
	myString = QTUtils_ConvertCToPascalString(kEffectsWindowTitle);
	MacSetRect(&myRect, kWindowOffset, kWindowOffset, kWindowOffset + kWidth, kWindowOffset + kHeight);
	gMainWindow = NewCWindow(NULL, &myRect, myString, true, 0, (WindowPtr)-1, true, 0);
	free(myString);
#endif
	
#if TARGET_OS_WIN32	
	// on Windows, we create an instance of a custom window class (gEffectsWindowClassName), associate a port
	// with it, and then keep track of that port in the global variable gMainWindow
	QTEffects_RegisterEffectsWindowClass(ghInst);
	myWindow = CreateWindow(gEffectsWindowClassName, kEffectsWindowTitle,
							WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_CHILD | WS_SYSMENU,
                            kWindowOffset, 
                            kWindowOffset,
                            kWidth + (2 * GetSystemMetrics(SM_CXBORDER)), 
                            kHeight + (2 * GetSystemMetrics(SM_CXBORDER)) + GetSystemMetrics(SM_CYCAPTION) - 1,
							ghWndMDIClient,
                            NULL, 
                            ghInst, 
                            NULL);
	if (myWindow != NULL)
		gMainWindow = (WindowPtr)CreatePortAssociation(myWindow, NULL, 0L);
		
#endif

bail:

#if TARGET_OS_MAC	
	// close down the splash screen
	if (myDialog != NULL)
		DisposeDialog(myDialog);
#endif

	// restore the cursor to the arrow
	InitCursor();
	if (myCursor != NULL)
		DisposeCCursor(myCursor);
		
	// now prompt the user for the first effect
	if ((gMainWindow != NULL) && (myErr == noErr))
		QTApp_HandleMenu(IDM_SELECT_EFFECT);

	return(myErr);
}


//////////
//
// QTEffects_Stop
// Do application-specific deinitialization for effects processing.
//
//////////

OSErr QTEffects_Stop (void)
{
	OSErr				myErr = noErr;

	// close the main effects window
#if TARGET_OS_WIN32
	if (gMainWindow != NULL)
		DestroyWindow(QTFrame_GetWindowReferenceFromPort(gMainWindow));
#endif
	
#if TARGET_OS_MAC
	if (gMainWindow != NULL)
		DisposeWindow(gMainWindow);
#endif

	if (gChooseDialog != NULL)
		DisposeDialog(gChooseDialog);

	// unhook the effects pop-up menu from the menu list and dispose of it
//	QTEffects_UninitializePopUpMenu(&gSelectEffectPopup);

	// deallocate any global storage
	if (gGW1Desc != NULL)
		DisposeHandle((Handle)gGW1Desc);
		
	if (gGW2Desc != NULL)
		DisposeHandle((Handle)gGW2Desc);

	if (gGW1 != NULL)
		DisposeGWorld(gGW1);
		
	if (gGW2 != NULL)
		DisposeGWorld(gGW2);
		
	if (gCurrentState.fSampleDescription != NULL)
		DisposeHandle((Handle)gCurrentState.fSampleDescription);
		
	if (gCurrentState.fEffectDescription != NULL)
		QTDisposeAtomContainer(gCurrentState.fEffectDescription);
		
	if (gCurrentState.fEffectSequenceID != 0L)
		CDSequenceEnd(gCurrentState.fEffectSequenceID);
		
	if (gCurrentState.fTimeBase != NULL)
		DisposeTimeBase(gCurrentState.fTimeBase);
		
	if (gCompInstance != NULL)
		CloseComponent(gCompInstance);

	return(myErr);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Effects window functions.
//
// Use these functions to set up and manage the main effects window displayed by this application.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////
//
// QTEffects_ProcessEffect
// Play one or more steps of the current effect.
// 
//////////

void QTEffects_ProcessEffect (void)
{
	OSErr		myErr = noErr;
	
	// if we are in "fast mode", play the effect forward thru to completion
	if (gCurrentState.fShowingEffect && gFastEffectDisplay) {
		for (gCurrentState.fTime = 1; gCurrentState.fTime <= gNumberOfSteps; gCurrentState.fTime++) {
			myErr = QTEffects_RunEffect(gCurrentState.fTime);
			if (myErr != noErr)
				return;
		}
		
		gCurrentState.fShowingEffect = false;
	}
	
	// otherwise, show the next step of the effect
	if (gCurrentState.fShowingEffect && !gFastEffectDisplay) {
	
		if (gCurrentDir == kForward) {
			gCurrentState.fTime++;
			if (gCurrentState.fTime > gNumberOfSteps) {
				switch (gLoopingState) {
					case kNoLooping:
						gCurrentState.fTime = 0;
						gCurrentDir = kForward;
						gCurrentState.fShowingEffect = false;
						break;
					case kNormalLooping:
						gCurrentState.fTime = 0;
						gCurrentDir = kForward;
						gCurrentState.fShowingEffect = true;
						break;
					case kPalindromeLooping:
						gCurrentState.fTime = gNumberOfSteps;
						gCurrentDir = kBackward;
						gCurrentState.fShowingEffect = true;
						break;
				}
			}
		} else {
			gCurrentState.fTime--;
			if (gCurrentState.fTime < 1) {
				switch (gLoopingState) {
					case kNoLooping:
						gCurrentState.fTime = 0;
						gCurrentDir = kForward;
						gCurrentState.fShowingEffect = false;
						break;
					case kPalindromeLooping:
					case kNormalLooping:		// (this should never actually happen, since we're going backward)
						gCurrentState.fTime = 0;
						gCurrentDir = kForward;
						gCurrentState.fShowingEffect = true;
						break;
				}
			}
		}
		
		// run the next step of the effect
		myErr = QTEffects_RunEffect(gCurrentState.fTime);
		if (myErr != noErr)
			return;

		if (gCurrentState.fSteppingEffect)
			gCurrentState.fShowingEffect = false;
	}
}


//////////
//
// QTEffects_DrawEffectsWindow
// Draw the contents of the main effects window.
// 
//////////

void QTEffects_DrawEffectsWindow (void)
{	
	if ((gCurrentState.fEffectSequenceID != 0L) && (gCurrentState.fTime != 1)) {
		QTEffects_RunEffect(gCurrentState.fTime);
	} else {
		// if we haven't set up an effect yet (which presumably happens only when the application is starting up)
		// or if we're at the first frame, just copy the first source image into the window
		CGrafPtr 		mySavedPort = NULL;
		GDHandle		mySavedGDevice = NULL;
		Rect			myRectGW1;
		Rect			myRectMain;
		
		// get the current port and device
		GetGWorld(&mySavedPort, &mySavedGDevice);
		
#if TARGET_OS_MAC
		MacSetPort((GrafPtr)GetWindowPort(gMainWindow));

		GetPortBounds(gGW1, &myRectGW1);
		GetPortBounds(GetWindowPort(gMainWindow), &myRectMain);
#endif
#if TARGET_OS_WIN32
		MacSetPort((GrafPtr)gMainWindow);

		myRectGW1 = gGW1->portRect;
		myRectMain = gMainWindow->portRect;
#endif
		
		// copy the image from the first source GWorld into the main window
		CopyBits(	(BitMapPtr)*GetGWorldPixMap(gGW1),
					(BitMapPtr)*GetGWorldPixMap(GetWindowPort(gMainWindow)),
					&myRectGW1,
					&myRectMain,
					srcCopy,
					NULL);

		// restore the original port and device
		SetGWorld(mySavedPort, mySavedGDevice);
	}
}


#if TARGET_OS_MAC
//////////
//
// QTEffects_HandleEffectsWindowEvents
// Handle Macintosh events for the main effects window.
// 
//////////

Boolean QTEffects_HandleEffectsWindowEvents (EventRecord *theEvent)
{
	Boolean			isHandled = false;
	short			myWindowPart;
	WindowPtr		myWindow;
	Rect			myRect;

	switch (theEvent->what) {
		case mouseDown:
			myWindowPart = MacFindWindow(theEvent->where, &myWindow);
			if (myWindow != gMainWindow)
				break;
				
			switch (myWindowPart) {
				case inDrag:
					GetRegionBounds(GetGrayRgn(), &myRect);
					DragWindow(myWindow, theEvent->where, &myRect);
					isHandled = true;
					break;
					
				case inContent:
					SelectWindow(myWindow);
					MacSetPort(GetWindowPort(myWindow));
					QTApp_HandleContentClick(myWindow, theEvent);
					isHandled = true;
					break;
					
				case inGoAway:
					// if the close box on the main window is clicked, dispose the window and quit the application
					if (TrackGoAway(myWindow, theEvent->where))
						if (myWindow == gMainWindow) {
							DisposeWindow(myWindow);
							gMainWindow = NULL;
							QTFrame_QuitFramework();
						}
					isHandled = true;
					break;
					
			} // end switch(myWindowPart)
			break;
	
		case updateEvt:
			myWindow = (WindowPtr)theEvent->message;
			if (myWindow == gMainWindow) {
				QTApp_Draw(myWindow);
				isHandled = true;
			}
			break;

		default:
			break;
	}
	
	return(isHandled);
}
#endif	// TARGET_OS_MAC


#if TARGET_OS_WIN32
//////////
//
// QTEffects_HandleEffectsWindowMessages
// Handle Windows messages for the main effects window.
// 
//////////

LRESULT CALLBACK QTEffects_HandleEffectsWindowMessages (HWND theWnd, UINT theMessage, UINT wParam, LONG lParam)
{
	PAINTSTRUCT		myPaintStruct;

	// run the next step(s) of the effect
	if (!gShuttingDown)
		QTEffects_ProcessEffect();
		
	switch (theMessage) {

		case WM_PAINT:
			BeginPaint(theWnd, &myPaintStruct);
			QTEffects_DrawEffectsWindow();
			EndPaint(theWnd, &myPaintStruct);
			break;

		case WM_NCLBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
			SetForegroundWindow(theWnd);
			break;
			
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			SetForegroundWindow(theWnd);
			QTApp_HandleContentClick(theWnd, NULL);
			return(0);

		case WM_CLOSE:
			if (theWnd != NULL)
				DestroyWindow(theWnd);
			gMainWindow = NULL;
			QTFrame_QuitFramework();
			break;

		case WM_DESTROY:
			DestroyPortAssociation((CGrafPtr)gMainWindow);
			break;

		default:
			break;
	}

	return(DefWindowProc(theWnd, theMessage, wParam, lParam));
}


//////////
//
// QTEffects_RegisterEffectsWindowClass
// Register the window procedure for the main effects window.
// 
//////////

BOOL QTEffects_RegisterEffectsWindowClass (HANDLE hInstance)
{
    WNDCLASSEX			myWC;

	// register the main effect window class
	myWC.cbSize        = sizeof(WNDCLASSEX);
	myWC.style         = CS_HREDRAW | CS_VREDRAW;
	myWC.lpfnWndProc   = (WNDPROC)QTEffects_HandleEffectsWindowMessages;
	myWC.cbClsExtra    = 0;
	myWC.cbWndExtra    = 0;
	myWC.hInstance     = hInstance;
	myWC.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
	myWC.hCursor       = LoadCursor(NULL, IDC_ARROW);
	myWC.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	myWC.lpszMenuName  = NULL;
	myWC.lpszClassName = gEffectsWindowClassName;
	myWC.hIconSm       = LoadImage(hInstance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, 0);
								 
	if (!RegisterClassEx(&myWC)) {
		if (!RegisterClass((LPWNDCLASS)&myWC.style))
    		return(false);
	}
	
	return(true);
}
#endif	// TARGET_OS_WIN32


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Dialog utilities.
//
// Use these functions to set up and manage the dialog boxes displayed by this application.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////
//
// QTEffects_LetUserChooseEffect
// Display a dialog box that contains a pop-up menu of all the effects currently available,
// and allow the user to select an effect.
// 
//////////

OSErr QTEffects_LetUserChooseEffect (void)
{
	GrafPtr 		mySavedPort = NULL;
	DialogPtr		myDialog = NULL;
	short			myItem;
	short			myType;
	Handle			myItemHandle = NULL;
	Rect			myRect;
	int				myIndex;
	OSErr			myErr = noErr;
	
	GetPort(&mySavedPort);

	if (gChooseDialog == NULL) {
		// get the dialog that lets the user select an effect component
		gChooseDialog = GetNewDialog(kSelectDialogID, NULL, (WindowPtr)-1);
		if (gChooseDialog == NULL)
			goto bail;
	} else {
#if TARGET_API_MAC_CARBON
		MacShowWindow(GetDialogWindow(gChooseDialog));
#else
		MacShowWindow(gChooseDialog);
#endif
	}
	
	myErr = SetDialogDefaultItem(gChooseDialog, kSelectButtonOK);
	if (myErr != noErr)
		goto bail;
		
	// find the currently selected effect in the list, so that it's showing when the dialog is displayed
	for (myIndex = 0; myIndex < gSelectEffectPopup.fNumberOfItems; myIndex++) {
		if (gSelectEffectPopup.fItemInfo[myIndex] == gCurrentState.fEffectType) {
			GetDialogItem(gChooseDialog, kSelectPopUpID, &myType, &myItemHandle, &myRect);
			SetControlValue((ControlHandle)myItemHandle, myIndex + 1);
		}
	}
	
	// now show the dialog
	MacShowWindow(GetDialogWindow(gChooseDialog));
	MacSetPort(GetDialogPort(gChooseDialog));
	
	do {
#if TARGET_OS_MAC
		ModalDialog(gModalFilterUPP, &myItem);
#endif
#if TARGET_OS_WIN32
		ModalDialog(NULL, &myItem);
#endif
	} while (myItem != kSelectButtonOK);
	
	// get the control handle for the pop-up menu and read its current value
	GetDialogItem(gChooseDialog, kSelectPopUpID, &myType, &myItemHandle, &myRect);
	gSelectEffectPopup.fLastChosen = GetControlValue((ControlHandle)myItemHandle);
		
	// now the user has hit OK and gSelectEffectPopup.fLastChosen contains the item chosen.
	gCurrentState.fEffectType = (OSType)gSelectEffectPopup.fItemInfo[gSelectEffectPopup.fLastChosen - 1];

bail:
	if (gChooseDialog != NULL)
#if TARGET_API_MAC_CARBON
		HideWindow(GetDialogWindow(gChooseDialog));
#else
		HideWindow(gChooseDialog);
#endif
	
	MacSetPort(mySavedPort);

	return(myErr);
}


//////////
//
// QTEffects_LetUserCustomizeEffect
// Display a dialog box that allows the user to select non-default parameters for an effect.
// 
//////////

OSErr QTEffects_LetUserCustomizeEffect (QTAtomContainer theEffectDesc)
{
	GrafPtr 				mySavedPort = NULL;
	ComponentDescription	myCD;
	Component				myComponent = NULL;
	QTAtomContainer			myParamDesc = NULL;
	MenuHandle				myMenu = NULL;
	OSErr					myErr = noErr;
		
	GetPort(&mySavedPort);

	// set up a component description
	myCD.componentType			= decompressorComponentType;	// effects are image decompressor components
	myCD.componentSubType		= gCurrentState.fEffectType;	// whichever subtype of effect we are looking for
	myCD.componentManufacturer	= 0;
	myCD.componentFlags			= 0;
	myCD.componentFlagsMask		= 0;
	
	// if the component has been previously opened, close it (since we may be customizing a different effect)
	if (gCompInstance != NULL) {
		CloseComponent(gCompInstance);
		gCompInstance = NULL;
	}
	
	// find the required component
	myComponent = FindNextComponent(myComponent, &myCD);
	if (myComponent == NULL)
		return(paramErr);
		
	// open the component
	gCompInstance = OpenComponent(myComponent);
	
	// get the list of parameters for the effect
	myErr = ImageCodecGetParameterList(gCompInstance, &myParamDesc);
	if (myErr != noErr)
		goto bail;
		
	// if the user has chosen the standard dialog box, then display it...
	if (gUseStandardDialog) {
		// set up the dialog box
		myErr = ImageCodecCreateStandardParameterDialog(
									gCompInstance,
									myParamDesc,
									theEffectDesc,
									0,							// dialog options
									NULL,						// no existing dialog
									0,							// no existing user item
									&gEffectsDialog);
		if ((myErr != noErr) || (gEffectsDialog == 0L))
			goto bail;
			
		gCustomDialog = NULL;
		
	} else {
	// ...otherwise, create the dialog box as a part of a custom application-supplied dialog box
	
		// Get the application-supplied dialog box; a few words of explanation are appropriate here:		
		// QuickTime expects this dialog box to be a color window, or the slider elements (and some
		// other controls) will not draw correctly; accordingly, your application needs to include
		// several resources that have the same ID as the dialog box resource (here, kCustomDialogID).
		// For MacOS 8 and/or Appearance Manager compatibility, the application must also a resource
		// of type 'dlgx', whose data (a long word) has at least the bits kDialogFlagsUseThemeBackground
		// and kDialogFlagsUseThemeControls set; we'll also set the bit kDialogFlagsHandleMovableModal.
		// So our entire 6-byte 'dlgx' resource is 00000000000D. (The first 2 bytes of the 'dlgx' resource
		// are a version number, which we set to 0.) For vanilla System 7 compatibility, the application
		// must include a resource of type 'dctb' whose ID is kCustomDialogID.

		gCustomDialog = GetNewDialog(kCustomDialogID, NULL, (WindowPtr)-1);
		if (gCustomDialog != NULL) {
		
			// set up the dialog box
			myErr = ImageCodecCreateStandardParameterDialog(
									gCompInstance,
									myParamDesc,
									theEffectDesc,
									0,							// dialog options
									gCustomDialog,				// the existing dialog
									kCustomUserItemID,			// the existing user item
									&gEffectsDialog);
			if ((myErr != noErr) || (gEffectsDialog == 0L))
				goto bail;
		
			SetDialogDefaultItem(gCustomDialog, kCustomButtonOK);
			
#if TARGET_OS_WIN32
			// on Windows, we need to explicitly tag the dialog box as a movable modal dialog box
			SetDialogMovableModal(gCustomDialog); 
#endif

			// see whether the current effect supports subpanels;
			// if so, display a pop-up menu that contains the names of those subpanels
			myErr = ImageCodecStandardParameterDialogDoAction(gCompInstance, gEffectsDialog, pdActionGetSubPanelMenu, &myMenu);
			if ((myErr == noErr) && (myMenu != NULL)) {
				short			myNumItems;
				short			myIndex;
				short			myItemKind;
				Handle			myItemHandle = NULL;
				Rect			myItemRect;
				
				// remove any existing menu items in gSubPanelPopUpMenu
				if (gSubPanelPopUpMenu != NULL) {
					myNumItems = CountMenuItems(gSubPanelPopUpMenu);
					for (myIndex = 0; myIndex < myNumItems; myIndex++)
						DeleteMenuItem(gSubPanelPopUpMenu, 1);
				}
				
				// get the control handle of the pop-up menu
				GetDialogItem(gCustomDialog, kCustomPopUpID, &myItemKind, &myItemHandle, &myItemRect);
				gSubPanelPopUpControl = (ControlHandle)myItemHandle;
				
				// get the menu handle of the pop-up menu
#if TARGET_API_MAC_CARBON
				gSubPanelPopUpMenu = GetControlPopupMenuHandle(gSubPanelPopUpControl);
#else
				gSubPanelPopUpMenu = (**(PopupPrivateDataHandle)(**gSubPanelPopUpControl).contrlData).mHandle;
#endif
				myNumItems = CountMenuItems(myMenu);
				for (myIndex = 1; myIndex <= myNumItems; myIndex++) {
					Str255		myString;
					
					// copy items into the pop-up menu
					GetMenuItemText(myMenu, myIndex, myString);
					MacInsertMenuItem(gSubPanelPopUpMenu, myString, myIndex + 1);
				}
				
				// set the minimum, maximum, and initial values of the pop-up menu
				SetControlMinimum(gSubPanelPopUpControl, 1);
				SetControlMaximum(gSubPanelPopUpControl, myNumItems);
				SetControlValue(gSubPanelPopUpControl, 1);
				ShowControl(gSubPanelPopUpControl);
			}

			MacShowWindow(GetDialogWindow(gCustomDialog));
			MacSetPort(GetDialogPort(gCustomDialog));
		}
	}
	
	// now, the frontmost window is a standard or custom effects parameter dialog box;
	// on the Mac, we call QTEffects_HandleEffectsDialogEvents in our main event loop
	// to find and process events targeted at the effects parameter dialog box; on Windows,
	// we need to use a different strategy: we install a modeless dialog callback procedure
	// that is called internally by QTML
#if TARGET_OS_WIN32
	SetModelessDialogCallbackProc(FrontWindow(), (QTModelessCallbackUPP)QTEffects_EffectsDialogCallback);
	QTMLSetWindowWndProc(FrontWindow(), QTEffects_CustomDialogWndProc);
#endif

bail:
	if (myParamDesc != NULL)
		QTDisposeAtomContainer(myParamDesc);

	MacSetPort(mySavedPort);

	return(myErr);
}


#if TARGET_OS_WIN32
//////////
//
// QTEffects_EffectsDialogCallback
// This function is called by QTML when it processes events for the standard or custom effects parameter dialog box.
// 
//////////

static void QTEffects_EffectsDialogCallback (EventRecord *theEvent, DialogRef theDialog, DialogItemIndex theItemHit)
{
	QTParamDialogEventRecord	myRecord;

	myRecord.theEvent = theEvent;
	myRecord.whichDialog = theDialog;
	myRecord.itemHit = theItemHit;

	if (gEffectsDialog != 0L) {
		ImageCodecStandardParameterDialogDoAction(gCompInstance, gEffectsDialog, pdActionModelessCallback, &myRecord);
	
		// see if the event is meant for the effects parameter dialog box
		QTEffects_HandleEffectsDialogEvents(theEvent, theItemHit);
	}
}


//////////
//
// QTEffects_CustomDialogWndProc
// Handle messages for the custom effects parameters dialog box.
// 
//////////

LRESULT CALLBACK QTEffects_CustomDialogWndProc (HWND theWnd, UINT theMessage, UINT wParam, LONG lParam)
{
	EventRecord			myEvent = {0};
	
	// pass idle events thru to the dialog callback
	if (theMessage == 0x7FFF)
		QTEffects_EffectsDialogCallback(&myEvent, GetNativeWindowPort(theWnd), 0);

	return(DefWindowProc(theWnd, theMessage, wParam, lParam));
}
#endif


//////////
//
// QTEffects_HandleEffectsDialogEvents
// Process events that might be targeted at the standard or custom effects parameter dialog box.
// Return true if the event was completely handled.
// 
//////////

Boolean QTEffects_HandleEffectsDialogEvents (EventRecord *theEvent, DialogItemIndex theItemHit)
{
#if TARGET_OS_MAC
#pragma unused(theItemHit)
#endif
	Boolean			isHandled = false;
	Boolean			myAcceptChanges = false;
	Boolean			myCloseDialog = false;
	OSErr			myErr = noErr;
	
	// handle events for our own items in the custom dialog box, if it exists;
	// we need to do this BEFORE we pass the event to ImageCodecIsStandardParameterDialogEvent
	if (gCustomDialog != NULL) {
		
#if TARGET_OS_WIN32
		// on Windows, QTML has already done any required control tracking for us, and
		// the theItemHit parameter contains the dialog item number of the selected item
		switch (theItemHit) {
			case kCustomPopUpID:
				ImageCodecStandardParameterDialogDoAction(gCompInstance, gEffectsDialog, pdActionActivateSubPanel, (void *)GetControlValue(gSubPanelPopUpControl));
				break;
			case kCustomButtonOK:
				myAcceptChanges = true;
				myCloseDialog = true;
				break;
			default:
				break;
		}
#endif
		
#if TARGET_OS_MAC
		// on the Mac, we need to do our own events processing
		switch (theEvent->what) {
			case mouseDown: {
				WindowPtr		myWindow;
				short			myPart;	
				ControlHandle	myControl;
				Point			myPoint;

				myPart = MacFindWindow(theEvent->where, &myWindow);
				if ((myWindow == GetDialogWindow(gCustomDialog)) && (myPart == inContent)) {
					// make sure we are the current drawing port (else GlobalToLocal won't work correctly)
					MacSetPort(GetDialogPort(gCustomDialog));
					
					myPoint = theEvent->where;
					GlobalToLocal(&myPoint);

					myPart = FindControl(myPoint, myWindow, &myControl);
					
					// first, see if the user clicked in the subpanel pop-up menu
					if ((myControl == gSubPanelPopUpControl) && (gSubPanelPopUpControl != NULL)) {
						TrackControl(myControl, myPoint, (ControlActionUPP)-1);
						ImageCodecStandardParameterDialogDoAction(gCompInstance, gEffectsDialog, pdActionActivateSubPanel, (void *)GetControlValue(myControl));
						return(true);
					}
					
					// next, see if the user clicked the OK button
					if (myPart == kControlButtonPart) {
						if (TrackControl(myControl, myPoint, NULL) != 0) {
							myAcceptChanges = true;
							myCloseDialog = true;
						}
					}
					
					// handle any other custom dialog box items here
					
				}
				break;
			}

			case keyDown:
			case autoKey:
				// handle the standard keyboard equivalents of OK button
				switch (theEvent->message & charCodeMask) {
					case kReturnKey:
					case kEnterKey:
						myAcceptChanges = true;
						myCloseDialog = true;
						break;
					default:
						break;
				}
				break;
		}
#endif
		
	}	// if (gCustomDialog != NULL)

	// pass the event to the standard effects parameter dialog box handler
	myErr = ImageCodecIsStandardParameterDialogEvent(gCompInstance, theEvent, gEffectsDialog);
	switch (myErr) {
	
		case codecParameterDialogConfirm:
			// the user has selected the OK button of the standard (not custom) effects parameter dialog box			
			myAcceptChanges = true;
			myCloseDialog = true;
			break;
			
		case userCanceledErr:
			// the user has selected the Cancel button of the standard (not custom) effects parameter dialog box
			myCloseDialog = true;
			break;
						
		case featureUnsupported:
			// the event was *not* handled by ImageCodecIsStandardParameterDialogEvent;
			// let the event be processed normally
			break;
		
		case noErr:
		default:
			// the event was completely handled by ImageCodecIsStandardParameterDialogEvent
			isHandled = true;
			break;
	}	

	if (myCloseDialog) {
        // retrieve the values from the parameters dialog box
		if (myAcceptChanges)
			ImageCodecStandardParameterDialogDoAction(gCompInstance, gEffectsDialog, pdActionConfirmDialog, NULL);
			
		// remove the dialog box
		ImageCodecDismissStandardParameterDialog(gCompInstance, gEffectsDialog);
		
		if (gCustomDialog != NULL)
			DisposeDialog(gCustomDialog);
			
		gCustomDialog = NULL;
		gEffectsDialog = 0L;
		
		isHandled = true;
		
		// now prepare the decompression sequence
		myErr = QTEffects_SetUpEffectSequence();
	}

	return(isHandled);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Pop-up menu utilities.
//
// Use these functions to set up and manage the global pop-up menu of effects used by this application.
// Here's our strategy: in our original resource file, the menu resource with ID kPopUpMenuID is empty;
// at run-time, we open the menu resource, remove any items from it, and then add a list of all effects
// to it. Then we write the new menu resource out to the resource file, whence GetNewDialog can find it.
// Very clever.
//
// Yeah, really clever...and not at all Carbonizable. Why? Apparently under Carbon, menu handles read from
// resources are not treated as resource handles. So calling ChangedResource and WriteResource no longer
// works. A better way to handle this is to follow the advice of Tech Note TB42 and insert the new menu
// into the menu list, whence the pop-up menu CDEF will retrieve it when GetNewDialog is called.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////
//
// QTEffects_InitializePopUpMenu
// Initialize the global pop-up effects menu.
// 
//////////

OSErr QTEffects_InitializePopUpMenu (PopUpMenuInformation *theMenuInfo)
{
	short			myIndex;
	short			myNumItems;
	OSErr			myErr = noErr;

	theMenuInfo->fMenu = MacGetMenu(kPopUpMenuID);
	if (theMenuInfo->fMenu == NULL) {
		myErr = ResError();
		goto bail;
	}
	
	// make sure the menu isn't purgeable
	HNoPurge((Handle)theMenuInfo->fMenu);
	
	// remove any existing menu items (since we're going to generate the effects list dynamically)
	myNumItems = CountMenuItems(theMenuInfo->fMenu);
	for (myIndex = 0; myIndex < myNumItems; myIndex++)
		DeleteMenuItem(theMenuInfo->fMenu, 1);
	
	theMenuInfo->fNumberOfItems = 0;
	theMenuInfo->fLastChosen = 1;

	// insert the menu into the menu list; the pop-up menu CDEF will find it there
	// when we open the dialog box containing this pop-up menu
	MacInsertMenu(theMenuInfo->fMenu, kInsertHierarchicalMenu);
	
bail:
	return(myErr);
}


//////////
//
// QTEffects_UninitializePopUpMenu
// Uninitialize the global pop-up effects menu.
// 
//////////

OSErr QTEffects_UninitializePopUpMenu (PopUpMenuInformation *theMenuInfo)
{
#if ACCESSOR_CALLS_ARE_FUNCTIONS
	MacDeleteMenu(GetMenuID(theMenuInfo->fMenu));
#else
	MacDeleteMenu((**theMenuInfo->fMenu).menuID);
#endif

	DisposeMenu(theMenuInfo->fMenu);
	
	return(noErr);
}


//////////
//
// QTEffects_AddItemToPopUpMenu
// Add an item to the global pop-up effects menu.
// 
//////////

OSErr QTEffects_AddItemToPopUpMenu (PopUpMenuInformation *theMenuInfo, char *theItemText, OSType theItemInfo)
{
	char		myCopy[256];
	StringPtr	myString = NULL;
	OSErr		myErr = noErr;

	strcpy(myCopy, theItemText);
	strcpy(theMenuInfo->fMenuText[theMenuInfo->fNumberOfItems], myCopy);
	
	myString = QTUtils_ConvertCToPascalString(myCopy);

	MacInsertMenuItem(theMenuInfo->fMenu, (unsigned char *)myString, theMenuInfo->fNumberOfItems);
	theMenuInfo->fItemInfo[theMenuInfo->fNumberOfItems] = theItemInfo;
	theMenuInfo->fNumberOfItems++;

	free(myString);
	return(myErr);
}


//////////
//
// QTEffects_AddListOfEffects
// Add a list of the available effects to the global pop-up effects menu.
// 
//////////

OSErr QTEffects_AddListOfEffects (void)
{
	QTAtomContainer			myEffectsList = NULL;
	short					myNumEffects;
	short					myIndex;
	OSErr					myErr = noErr;
	
	// get a list of the available effects
	myErr = QTNewAtomContainer(&myEffectsList);
	if (myErr != noErr)
		goto bail;
		
	myErr = QTGetEffectsList(&myEffectsList, kNoMinNumSources, kNoMaxNumSources, 0L);
	if (myErr != noErr)
		goto bail;

	// the returned effects list contains (at least) two atoms for each available effect component,
	// a name atom and a type atom; happily, this list is already sorted alphabetically by effect name
	myNumEffects = QTCountChildrenOfType(myEffectsList, kParentAtomIsContainer, kEffectNameAtom);
	for (myIndex = 1; myIndex <= myNumEffects; myIndex++) {
		QTAtom				myNameAtom = 0L;
		QTAtom				myTypeAtom = 0L;

		myNameAtom = QTFindChildByIndex(myEffectsList, kParentAtomIsContainer, kEffectNameAtom, myIndex, NULL);
		myTypeAtom = QTFindChildByIndex(myEffectsList, kParentAtomIsContainer, kEffectTypeAtom, myIndex, NULL);
		if ((myNameAtom != 0L) && (myTypeAtom != 0L)) {
			char 			myName[256];
			OSType 			myType;
			long			mySize;

			// get the data from the type and name atoms
			QTCopyAtomDataToPtr(myEffectsList, myTypeAtom, false, sizeof(myType), &myType, NULL);
			QTCopyAtomDataToPtr(myEffectsList, myNameAtom, true, sizeof(myName), myName, &mySize);
			myName[mySize] = '\0';
			
			QTEffects_AddItemToPopUpMenu(&gSelectEffectPopup, myName, myType);
		}
	}
		
bail:
	QTDisposeAtomContainer(myEffectsList);	
	return(myErr);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Effects utilities.
//
// Use these functions to set up and run QuickTime video effects.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////
//
// QTEffects_CreateEffectDescription
// Create an effect description for zero, one, or two sources.
// 
// The effect description specifies which video effect is desired and the parameters for that effect.
// It also describes the source(s) for the effect. An effect description is simply an atom container
// that holds atoms with the appropriate information.
//
// Note that because we are creating an atom container, we must pass big-endian data (hence the calls
// to EndianU32_NtoB).
//
// The caller is responsible for disposing of the returned atom container, by calling QTDisposeAtomContainer.
//
//////////

QTAtomContainer QTEffects_CreateEffectDescription (OSType theEffectName, OSType theSourceName1, OSType theSourceName2)
{
	QTAtomContainer		myEffectDesc = NULL;
	OSType				myType;
	OSErr				myErr = noErr;

	// create a new, empty effect description
	myErr = QTNewAtomContainer(&myEffectDesc);
	if (myErr != noErr)
		goto bail;

	// create the effect ID atom: the atom type is kParameterWhatName, and the atom ID is kParameterWhatID
	myType = EndianU32_NtoB(theEffectName);
	myErr = QTInsertChild(myEffectDesc, kParentAtomIsContainer, kParameterWhatName, kParameterWhatID, 0, sizeof(myType), &myType, NULL);
	if (myErr != noErr)
		goto bail;
		
	// add the first source, if it's not kSourceNoneName
	if (theSourceName1 != kSourceNoneName) {
		myType = EndianU32_NtoB(theSourceName1);
		myErr = QTInsertChild(myEffectDesc, kParentAtomIsContainer, kEffectSourceName, 1, 0, sizeof(myType), &myType, NULL);
		if (myErr != noErr)
			goto bail;
	}
							
	// add the second source, if it's not kSourceNoneName
	if (theSourceName2 != kSourceNoneName) {
		myType = EndianU32_NtoB(theSourceName2);
		myErr = QTInsertChild(myEffectDesc, kParentAtomIsContainer, kEffectSourceName, 2, 0, sizeof(myType), &myType, NULL);
	}

bail:
	return(myEffectDesc);
}


//////////
//
// QTEffects_AddTrackReferenceToInputMap
// Add a track reference to the specified input map.
// 
//////////

OSErr QTEffects_AddTrackReferenceToInputMap (QTAtomContainer theInputMap, Track theTrack, Track theSrcTrack, OSType theSrcName)
{
	OSErr				myErr = noErr;
	QTAtom				myInputAtom;
	long				myRefIndex;
	OSType				myType;

	myErr = AddTrackReference(theTrack, theSrcTrack, kTrackModifierReference, &myRefIndex);
	if (myErr != noErr)
		goto bail;
			
	// add a reference atom to the input map
	myErr = QTInsertChild(theInputMap, kParentAtomIsContainer, kTrackModifierInput, myRefIndex, 0, 0, NULL, &myInputAtom);
	if (myErr != noErr)
		goto bail;
	
	// add two child atoms to the parent reference atom
	myType = EndianU32_NtoB(kTrackModifierTypeImage);
	myErr = QTInsertChild(theInputMap, myInputAtom, kTrackModifierType, 1, 0, sizeof(myType), &myType, NULL);
	if (myErr != noErr)
		goto bail;
	
	myType = EndianU32_NtoB(theSrcName);
	myErr = QTInsertChild(theInputMap, myInputAtom, kEffectDataSourceType, 1, 0, sizeof(myType), &myType, NULL);
		
bail:
	return(myErr);
}
 
 
//////////
//
// QTEffects_SetUpEffectSequence
// Set up an effects sequence.
// 
//////////

OSErr QTEffects_SetUpEffectSequence (void)
{
	OSErr						myErr = noErr;
	ImageSequenceDataSource		mySrc1 = 0;
	ImageSequenceDataSource		mySrc2 = 0;
	ImageSequenceDataSource		mySrc3 = 0;
	PixMapHandle				mySrcPixMap;
	PixMapHandle				myDstPixMap;
 	
	// if an effect sequence is already set up, end it
	if (gCurrentState.fEffectSequenceID != 0L) {
		CDSequenceEnd(gCurrentState.fEffectSequenceID);
		gCurrentState.fEffectSequenceID = 0L;
	}
	
	// if there is a timebase already set up, dispose of it
	if (gCurrentState.fTimeBase != NULL) {
		DisposeTimeBase(gCurrentState.fTimeBase);
		gCurrentState.fTimeBase = NULL;
	}
		
	// make an effects sequence
	HLock((Handle)gCurrentState.fEffectDescription);

	// prepare the decompression sequence for playback
	myErr = DecompressSequenceBeginS(
							&gCurrentState.fEffectSequenceID,
							gCurrentState.fSampleDescription,
#if TARGET_CPU_68K
							StripAddress(*gCurrentState.fEffectDescription),
#else
							*gCurrentState.fEffectDescription,
#endif
							GetHandleSize(gCurrentState.fEffectDescription),
							(CGrafPtr)GetWindowPort(gMainWindow),
							NULL,
							NULL,
							NULL,
							ditherCopy,
							NULL,
							0,
							codecNormalQuality,
							NULL);

	HUnlock((Handle)gCurrentState.fEffectDescription);
	if (myErr != noErr)
		goto bail;

	// get the pixel maps for the GWorlds
	mySrcPixMap = GetGWorldPixMap(gGW1);
	myDstPixMap = GetGWorldPixMap(gGW2);
	
	if ((mySrcPixMap == NULL) || (myDstPixMap == NULL))
		goto bail;

	// make the first effect source
	if (gGW1 == NULL)
		goto bail;
	myErr = MakeImageDescriptionForPixMap(mySrcPixMap, &gGW1Desc);
	if (myErr != noErr)
		goto bail;

	myErr = CDSequenceNewDataSource(gCurrentState.fEffectSequenceID, &mySrc1, kSourceOneName, 1, (Handle)gGW1Desc, NULL, 0);
	if (myErr != noErr)
		goto bail;

	CDSequenceSetSourceData(mySrc1, GetPixBaseAddr(mySrcPixMap), (**gGW1Desc).dataSize);

	// make the second effect source
	if (gGW2 == NULL)
		goto bail;
	myErr = MakeImageDescriptionForPixMap(myDstPixMap, &gGW2Desc);
	if (myErr != noErr)
		goto bail;

	myErr = CDSequenceNewDataSource(gCurrentState.fEffectSequenceID, &mySrc2, kSourceTwoName, 1, (Handle)gGW2Desc, NULL, 0);
	if (myErr != noErr)
		goto bail;

	CDSequenceSetSourceData(mySrc2, GetPixBaseAddr(myDstPixMap), (**gGW2Desc).dataSize);

	// create a new time base and associate it with the decompression sequence
	gCurrentState.fTimeBase = NewTimeBase();
	myErr = GetMoviesError();
	if (myErr != noErr)
		goto bail;

	SetTimeBaseRate(gCurrentState.fTimeBase, 0);
	myErr = CDSequenceSetTimeBase(gCurrentState.fEffectSequenceID, gCurrentState.fTimeBase);

bail:
	return(myErr);
}


//////////
//
// QTEffects_MakeSampleDescription
// Return a new image description with default and specified values.
// 
//////////

ImageDescriptionHandle QTEffects_MakeSampleDescription (OSType theEffectType, short theWidth, short theHeight)
{
	ImageDescriptionHandle		mySampleDesc = NULL;

#if USES_MAKE_IMAGE_DESC_FOR_EFFECT
	OSErr						myErr = noErr;
	
	// create a new sample description
	myErr = MakeImageDescriptionForEffect(theEffectType, &mySampleDesc);
	if (myErr != noErr)
		return(NULL);
#else
	// create a new sample description
	mySampleDesc = (ImageDescriptionHandle)NewHandleClear(sizeof(ImageDescription));
	if (mySampleDesc == NULL)
		return(NULL);
		
	// fill in the fields of the sample description
	(**mySampleDesc).cType = theEffectType;
	(**mySampleDesc).idSize = sizeof(ImageDescription);
	(**mySampleDesc).hRes = 72L << 16;
	(**mySampleDesc).vRes = 72L << 16;
	(**mySampleDesc).frameCount = 1;
	(**mySampleDesc).depth = 0;
	(**mySampleDesc).clutID = -1;
#endif
	
	(**mySampleDesc).vendor = kAppleManufacturer;
	(**mySampleDesc).temporalQuality = codecNormalQuality;
	(**mySampleDesc).spatialQuality = codecNormalQuality;
	(**mySampleDesc).width = theWidth;
	(**mySampleDesc).height = theHeight;
	
	return(mySampleDesc);
}


//////////
//
// QTEffects_RunEffect
// Run the effect: decompress a single step of the effect sequence.
// 
//////////

OSErr QTEffects_RunEffect (TimeValue theTime)
{
	OSErr						myErr = noErr;
	ICMFrameTimeRecord			myFrameTime;

	// assertions
	if ((gCurrentState.fEffectDescription == NULL) || (gCurrentState.fEffectSequenceID == 0L))
		goto bail;

	// set the timebase time to the step of the sequence to be rendered
	SetTimeBaseValue(gCurrentState.fTimeBase, theTime, gNumberOfSteps);

	myFrameTime.value.hi				= 0;
	myFrameTime.value.lo				= theTime;
	myFrameTime.scale					= gNumberOfSteps;
	myFrameTime.base					= 0;
	myFrameTime.duration				= gNumberOfSteps;
	myFrameTime.rate					= 0;
	myFrameTime.recordSize				= sizeof(myFrameTime);
	myFrameTime.frameNumber				= 1;
	myFrameTime.flags					= icmFrameTimeHasVirtualStartTimeAndDuration;
	myFrameTime.virtualStartTime.lo		= 0;
	myFrameTime.virtualStartTime.hi		= 0;
	myFrameTime.virtualDuration			= gNumberOfSteps;
	
	HLock((Handle)gCurrentState.fEffectDescription);

	myErr = DecompressSequenceFrameWhen(
										gCurrentState.fEffectSequenceID,
#if TARGET_CPU_68K
										StripAddress(*((Handle)gCurrentState.fEffectDescription)),
#else
										*((Handle)gCurrentState.fEffectDescription),
#endif
										GetHandleSize((Handle)gCurrentState.fEffectDescription),
										0,
										0,
										NULL,
										&myFrameTime);
										
	HUnlock((Handle)gCurrentState.fEffectDescription);
	
	if (myErr != noErr)
		goto bail;
	
bail:
	return(myErr);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// General imaging utilities.
//
// Use these functions to draw pictures into GWorlds and do other imaging operations.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////
//
// QTEffects_GetPictResourceAsGWorld
// Create a new GWorld of the specified size and bit depth; then draw the specified PICT resource into it.
// The new GWorld is returned through the theGW parameter.
//
//////////

OSErr QTEffects_GetPictResourceAsGWorld (short theResID, short theWidth, short theHeight, short theDepth, GWorldPtr *theGW)
{
	PicHandle				myHandle = NULL;
	PixMapHandle			myPixMap = NULL;
	CGrafPtr				mySavedPort;
	GDHandle				mySavedDevice;
	Rect					myRect;
	OSErr					myErr = noErr;

	// get the current drawing environment
	GetGWorld(&mySavedPort, &mySavedDevice);

	// read the specified PICT resource from the application's resource file
	myHandle = GetPicture(theResID);
	if (myHandle == NULL) {
		myErr = ResError();
		if (myErr == noErr)
			myErr = resNotFound;
		goto bail;
	}

	// set the size of the GWorld
	MacSetRect(&myRect, 0, 0, theWidth, theHeight);

	// allocate a new GWorld
	myErr = QTNewGWorld(theGW, theDepth, &myRect, NULL, NULL, kICMTempThenAppMemory);
	if (myErr != noErr)
		goto bail;
	
	SetGWorld(*theGW, NULL);

	// get a handle to the offscreen pixel image and lock it
	myPixMap = GetGWorldPixMap(*theGW);
	LockPixels(myPixMap);

	EraseRect(&myRect);
	DrawPicture(myHandle, &myRect);
	
	if (myPixMap != NULL)
		UnlockPixels(myPixMap);
	
bail:
	// restore the previous port and device
	SetGWorld(mySavedPort, mySavedDevice);

	if (myHandle != NULL)
		ReleaseResource((Handle)myHandle);
	
	return(myErr);
}


//////////
//
// QTEffects_GetPictureAsGWorld
// Prompt the user to select a picture, create a new GWorld of the specified size and bit depth,
// and then draw the picture into it. The new GWorld is returned through the theGW parameter.
//
//////////

OSErr QTEffects_GetPictureAsGWorld (short theWidth, short theHeight, short theDepth, GWorldPtr *theGW)
{
	FSSpec						myFSSpec;
	OSType 						myTypeList[] = {kQTFileTypeQuickTimeImage};
	short						myNumTypes = 1;
	GraphicsImportComponent		myImporter = NULL;
	Rect						myRect;
	QTFrameFileFilterUPP		myFileFilterUPP = NULL;
	OSErr						myErr = paramErr;

#if TARGET_OS_MAC
	myNumTypes = 0;
#endif

	// have the user select an image file
	myFileFilterUPP = QTFrame_GetFileFilterUPP((ProcPtr)QTFrame_FilterFiles);
	myErr = QTFrame_GetOneFileWithPreview(myNumTypes, (QTFrameTypeListPtr)myTypeList, &myFSSpec, myFileFilterUPP);
	if (myErr != noErr)
		goto bail;
		
	// get a graphics importer for the image file
	myErr = GetGraphicsImporterForFile(&myFSSpec, &myImporter);
	if (myErr != noErr)
		goto bail;

	if (*theGW != NULL) {
		DisposeGWorld(*theGW);
		*theGW = NULL;
	}
	
	// set the size of the GWorld
	MacSetRect(&myRect, 0, 0, theWidth, theHeight);

	// allocate a new GWorld
	myErr = QTNewGWorld(theGW, theDepth, &myRect, NULL, NULL, kICMTempThenAppMemory);
	if (myErr != noErr)
		goto bail;
	
	// draw the picture into the GWorld
	GraphicsImportSetGWorld(myImporter, *theGW, NULL);
	GraphicsImportSetBoundsRect(myImporter, &myRect);
	GraphicsImportDraw(myImporter);

bail:
	if (myFileFilterUPP != NULL)
		DisposeNavObjectFilterUPP(myFileFilterUPP);

	if (myImporter != NULL)
		CloseComponent(myImporter);
	
	return(myErr);
}


//////////
//
// QTEffects_AddVideoTrackFromGWorld
// Add to the specified movie a video track for the specified picture resource.
//
//////////

OSErr QTEffects_AddVideoTrackFromGWorld (Movie *theMovie, GWorldPtr theGW, Track *theSourceTrack, long theStartTime, short theWidth, short theHeight)
{
	Media						myMedia;
	ImageDescriptionHandle		myDesc = NULL;
	Rect						myRect;
	Rect						myRect2;
	Rect						myRect3;
	long						mySize;
	Handle						myData = NULL;
	Ptr							myDataPtr = NULL;
	GWorldPtr					myGWorld = NULL;
	CGrafPtr 					mySavedPort = NULL;
	GDHandle 					mySavedGDevice = NULL;
	PicHandle					myHandle = NULL;
	PixMapHandle				mySrcPixMap = NULL;
	PixMapHandle				myDstPixMap = NULL;
	OSErr						myErr = noErr;
	
	// get the current port and device
	GetGWorld(&mySavedPort, &mySavedGDevice);
	
	// create a video track in the movie
	*theSourceTrack = NewMovieTrack(*theMovie, FixRatio(theWidth, 1), FixRatio(theHeight, 1), kNoVolume);
	myMedia = NewTrackMedia(*theSourceTrack, VideoMediaType, kOneSecond, NULL, 0);
	
	// get the rectangle for the movie
	GetMovieBox(*theMovie, &myRect);
	
	// begin editing the new track
	BeginMediaEdits(myMedia);
		
	// create a new GWorld; we draw the picture into this GWorld and then compress it
	// (note that we are creating a picture with the maximum bit depth)
	myErr = NewGWorld(&myGWorld, 32, &myRect, NULL, NULL, 0L);
	if (myErr != noErr)
		goto bail;
	
	mySrcPixMap = GetGWorldPixMap(theGW);
	// LockPixels(mySrcPixMap);
	
	myDstPixMap = GetGWorldPixMap(myGWorld);
	LockPixels(myDstPixMap);
	
	// create a new image description; CompressImage will fill in the fields of this structure
	myDesc = (ImageDescriptionHandle)NewHandle(4);
	
	SetGWorld(myGWorld, NULL);
#if TARGET_OS_MAC
	GetPortBounds(theGW, &myRect2);
	GetPortBounds(myGWorld, &myRect3);
#endif
#if TARGET_OS_WIN32
	myRect2 = theGW->portRect;
	myRect3 = myGWorld->portRect;
#endif

	// copy the image from the specified GWorld into the new GWorld
	CopyBits(	
				(BitMapPtr)*mySrcPixMap,
				(BitMapPtr)*myDstPixMap,
				&myRect2,
				&myRect3,
				srcCopy,
				NULL);

	// restore the original port and device
	SetGWorld(mySavedPort, mySavedGDevice);
	
	myErr = GetMaxCompressionSize(myDstPixMap, &myRect, 0, codecNormalQuality, kAnimationCodecType, anyCodec, &mySize);
	if (myErr != noErr)
		goto bail;
		
	myData = NewHandle(mySize);
	if (myData == NULL)
		goto bail;
		
	HLockHi(myData);
#if TARGET_CPU_68K
	myDataPtr = StripAddress(*myData);
#else
	myDataPtr = *myData;
#endif
	myErr = CompressImage(myDstPixMap, &myRect, codecNormalQuality, kAnimationCodecType, myDesc, myDataPtr);
	if (myErr != noErr)
		goto bail;
		
	myErr = AddMediaSample(myMedia, myData, 0, (**myDesc).dataSize, kEffectMovieDuration, (SampleDescriptionHandle)myDesc, 1, 0, NULL);
	if (myErr != noErr)
		goto bail;

	myErr = EndMediaEdits(myMedia);
	if (myErr != noErr)
		goto bail;

	myErr = InsertMediaIntoTrack(*theSourceTrack, theStartTime, 0, GetMediaDuration(myMedia), fixed1);
	
bail:
	// restore the original port and device
	SetGWorld(mySavedPort, mySavedGDevice);
	
	if (myData != NULL) {
		HUnlock(myData);
		DisposeHandle(myData);
	}

	if (myDesc != NULL)
		DisposeHandle((Handle)myDesc);
		
	// if (mySrcPixMap != NULL)
	// 	UnlockPixels(mySrcPixMap);
		
	if (myDstPixMap != NULL)
		UnlockPixels(myDstPixMap);
		
	if (myGWorld != NULL)
		DisposeGWorld(myGWorld);
	
	return(myErr);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Movie utilities.
//
// Use these functions to create movie files and do other movie operations.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////
//
// QTEffects_CreateEffectsMovie
// Create a movie containing the current video effect transition from one picture to another.
//
//////////

void QTEffects_CreateEffectsMovie (OSType theEffectType, QTAtomContainer theEffectDesc, short theWidth, short theHeight)
{
	ImageDescriptionHandle	mySampleDesc = NULL;
	short					myResRefNum = 0;
	short					myResID = movieInDataForkResID;
	Movie					myMovie = NULL;
	Track					myTrack = NULL;
	Track					mySrc1Track = NULL;
	Track					mySrc2Track = NULL;
	Media					myMedia;
	FSSpec					myFile;
	Boolean					myIsSelected = false;
	Boolean					myIsReplacing = false;	
	QTAtomContainer			myInputMap = NULL;
	TimeValue				mySampleTime;
	long					myFlags = createMovieFileDeleteCurFile | createMovieFileDontCreateResFile;
	StringPtr 				myMoviePrompt = QTUtils_ConvertCToPascalString(kSaveEffectMoviePrompt);
	StringPtr 				myMovieFileName = QTUtils_ConvertCToPascalString(kSaveEffectMovieFileName);
	OSErr					myErr = noErr;
	
	// create an effect sample description
	mySampleDesc = QTEffects_MakeSampleDescription(theEffectType, theWidth, theHeight);
	if (mySampleDesc == NULL)
		goto bail;

	// prompt user for new file name
	QTFrame_PutFile(myMoviePrompt, myMovieFileName, &myFile, &myIsSelected, &myIsReplacing);
	if (!myIsSelected)
		goto bail;				// deal with user cancelling

	// create a movie file for the destination movie
	myErr = CreateMovieFile(&myFile, sigMoviePlayer, smCurrentScript, myFlags, &myResRefNum, &myMovie);
	if (myErr != noErr)
		goto bail;
	
	// add the video tracks of the source pictures to the effects movie;
	// the video tracks used as sources for the effect should start at the same time as the effect track
	// and end at the same time as the effect track
	myErr = QTEffects_AddVideoTrackFromGWorld(&myMovie, gGW1, &mySrc1Track, 0, theWidth, theHeight);
	if (myErr != noErr)
		goto bail;
		
	myErr = QTEffects_AddVideoTrackFromGWorld(&myMovie, gGW2, &mySrc2Track, 0, theWidth, theHeight);
	if (myErr != noErr)
		goto bail;

	// create the video effect track and media
	myTrack = NewMovieTrack(myMovie, FixRatio(theWidth, 1), FixRatio(theHeight, 1), kNoVolume);
	myMedia = NewTrackMedia(myTrack, VideoMediaType, kOneSecond, NULL, 0);

	// add the effect description as a sample to the effect track media
	BeginMediaEdits(myMedia);

	myErr = AddMediaSample(myMedia, (Handle)theEffectDesc, 0, GetHandleSize((Handle)theEffectDesc), kEffectMovieDuration, (SampleDescriptionHandle)mySampleDesc, 1, 0, &mySampleTime);
	if (myErr != noErr)
		goto bail;

	EndMediaEdits(myMedia);
	
	// create the input map and add references for the two video tracks
	myErr = QTNewAtomContainer(&myInputMap);
	if (myErr != noErr)
		goto bail;
		
	myErr = QTEffects_AddTrackReferenceToInputMap(myInputMap, myTrack, mySrc1Track, kSourceOneName);
	if (myErr != noErr)
		goto bail;
		
	myErr = QTEffects_AddTrackReferenceToInputMap(myInputMap, myTrack, mySrc2Track, kSourceTwoName);
	if (myErr != noErr)
		goto bail;

	// add the input map to the effects track
	myErr = SetMediaInputMap(myMedia, myInputMap);
	if (myErr != noErr)
		goto bail;

	// add the media to the track
	myErr = InsertMediaIntoTrack(myTrack, 0, mySampleTime, GetMediaDuration(myMedia), fixed1);
	if (myErr != noErr)
		goto bail;
		
#if ALLOW_COMPOUND_EFFECTS
	QTEffects_AddFilmNoiseToMovie(myMovie, myTrack);
#endif

#ifdef __QTUtilities__
	// save the current looping state
	myErr = QTUtils_SetMovieFileLoopingInfo(myMovie, (gLoopingState - (kSettingsMenuResID << 8)) - 2);
#endif

	// put the movie resource into the file
	myErr = AddMovieResource(myMovie, myResRefNum, &myResID, NULL);
	
bail:		
	if (mySampleDesc != NULL)
		DisposeHandle((Handle)mySampleDesc);
	
	if (myResRefNum != 0)
		CloseMovieFile(myResRefNum);

	if (myMovie != NULL)
		DisposeMovie(myMovie);

	if (myInputMap != NULL)
		QTDisposeAtomContainer(myInputMap);
	
	free(myMoviePrompt);
	free(myMovieFileName);

	return;
}


//////////
//
// QTEffects_AddFilmNoiseToMovie
// Add the film noise filter to the specified track.
//
//////////

void QTEffects_AddFilmNoiseToMovie (Movie theMovie, Track theSrcTrack)
{
	ImageDescriptionHandle	mySampleDesc = NULL;
	Track					myTrack = NULL;
	Media					myMedia = NULL;
	QTAtomContainer			myInputMap = NULL;
	QTAtomContainer			myEffectDesc = NULL;
	TimeValue				mySampleTime;
	Fixed					myWidth, myHeight;
	OSErr					myErr = noErr;

	// get width and height of track
	GetTrackDimensions(theSrcTrack, &myWidth, &myHeight);	
	
	// create an effect sample description
	mySampleDesc = QTEffects_MakeSampleDescription(kFilmNoiseImageFilterType, myWidth >> 16, myHeight >> 16);
	if (mySampleDesc == NULL)
		goto bail;

	// create the video effect track and media
	myTrack = NewMovieTrack(theMovie, myWidth, myHeight, kNoVolume);
	myMedia = NewTrackMedia(myTrack, VideoMediaType, kOneSecond, NULL, 0);

	// create an effect description
	myEffectDesc = QTEffects_CreateEffectDescription(kFilmNoiseImageFilterType, kSourceThreeName, kSourceNoneName);

	// add the effect description as a sample to the effect track media
	BeginMediaEdits(myMedia);

	myErr = AddMediaSample(myMedia, (Handle)myEffectDesc, 0, GetHandleSize((Handle)myEffectDesc), kEffectMovieDuration, (SampleDescriptionHandle)mySampleDesc, 1, 0, &mySampleTime);
	if (myErr != noErr)
		goto bail;

	EndMediaEdits(myMedia);
	
	// create the input map and add references for the first effect track
	myErr = QTNewAtomContainer(&myInputMap);
	if (myErr != noErr)
		goto bail;
		
	myErr = QTEffects_AddTrackReferenceToInputMap(myInputMap, myTrack, theSrcTrack, kSourceThreeName);
	if (myErr != noErr)
		goto bail;
		
	// add the input map to the effects track
	myErr = SetMediaInputMap(myMedia, myInputMap);
	if (myErr != noErr)
		goto bail;

	// add the media to the track
	myErr = InsertMediaIntoTrack(myTrack, 0, mySampleTime, GetMediaDuration(myMedia), fixed1);
	if (myErr != noErr)
		goto bail;
	
bail:		
	if (mySampleDesc != NULL)
		DisposeHandle((Handle)mySampleDesc);
	
	if (myInputMap != NULL)
		QTDisposeAtomContainer(myInputMap);
	
	return;
}
