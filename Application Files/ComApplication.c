//////////
//
//	File:		ComApplication.c
//
//	Contains:	Application-specific code for basic QuickTime movie display and control.
//
//	Written by:	Tim Monroe
//				Based on the QTShell code written by Tim Monroe, which in turn was based on the MovieShell
//				code written by Kent Sandvik (Apple DTS). This current version is now very far removed from
//				MovieShell.
//
//	Copyright:	� 1999 by Apple Computer, Inc., all rights reserved.
//
//	Change History (most recent first):
//	   
//	   <5>	 	03/20/00	rtm		made changes to get things running under CarbonLib
//	   <4>	 	02/20/98	rtm		revised custom dialog box handling; now works on Windows (yippee!)
//	   <3>	 	02/12/98	rtm		added support for stepping through the effect
//	   <2>	 	12/18/97	rtm		added low-level effects calls for QTShowEffect
//	   <1>	 	12/21/94	khs		first file
//
//////////

//////////
//
// header files
//
//////////

#include "ComApplication.h"
#include "QTShowEffect.h"


//////////
//
// global variables
//
//////////

#if TARGET_OS_MAC
AEEventHandlerUPP		gHandleOpenAppAEUPP;					// UPPs for our Apple event handlers
AEEventHandlerUPP		gHandleOpenDocAEUPP;
AEEventHandlerUPP		gHandlePrintDocAEUPP;
AEEventHandlerUPP		gHandleQuitAppAEUPP;
extern Boolean			gAppInForeground;						// is our application in the foreground?	
#endif

#if TARGET_OS_WIN32
extern HWND				ghWnd;									// the MDI frame window; this window has the menu bar
#endif

// external variables
extern QTParameterDialog	gEffectsDialog;
//extern DialogPtr			gCustomDialog;
extern StateInformation		gCurrentState;
extern Boolean				gUseStandardDialog;
extern Boolean				gFastEffectDisplay;
extern int					gNumberOfSteps;
extern unsigned short		gLoopingState;
extern unsigned short		gCurrentDir;
extern WindowPtr			gMainWindow;
extern GWorldPtr			gGW1;
extern GWorldPtr			gGW2;
extern GWorldPtr			gGW3;


//////////
//
// QTApp_Init
// Do any application-specific initialization.
//
// The theStartPhase parameter determines which "phase" of application start-up is executed,
// *before* the MDI frame window is created or *after*. This distinction is relevant only on
// Windows, so on MacOS, you should always use kInitAppPhase_BothPhases.
//
//////////

void QTApp_Init (UInt32 theStartPhase)
{
	// do any start-up activities that should occur before the MDI frame window is created
	if (theStartPhase & kInitAppPhase_BeforeCreateFrameWindow) {
#if TARGET_OS_MAC
		// check to make sure that QuickTime video effects are available;
		// we depend on these features  
		if (!QTUtils_HasQuickTimeVideoEffects())
			QTFrame_QuitFramework();

		// make sure that the Apple Event Manager is available; install handlers for required Apple events
		QTApp_InstallAppleEventHandlers();
#endif
	}

	// do any start-up activities that should occur after the MDI frame window is created
	if (theStartPhase & kInitAppPhase_AfterCreateFrameWindow) {
#if TARGET_OS_WIN32
		// on Windows, open as movie documents any files specified on the command line
		SendMessage(ghWnd, WM_OPENDROPPEDFILES, 0L, 0L);
#endif

		// initialize for QuickTime effects
		QTEffects_Init();
	}
}


//////////
//
// QTApp_Stop
// Do any application-specific shut-down.
//
// The theStopPhase parameter determines which "phase" of application shut-down is executed,
// *before* any open movie windows are destroyed or *after*.
//
//////////

void QTApp_Stop (UInt32 theStopPhase)
{	
	// do any shut-down activities that should occur before the movie windows are destroyed
	if (theStopPhase & kStopAppPhase_BeforeDestroyWindows) {
		QTEffects_Stop();
	}
	
	// do any shut-down activities that should occur after the movie windows are destroyed
	if (theStopPhase & kStopAppPhase_AfterDestroyWindows) {
#if TARGET_OS_MAC
		// dispose of routine descriptors for Apple event handlers
		DisposeAEEventHandlerUPP(gHandleOpenAppAEUPP);
		DisposeAEEventHandlerUPP(gHandleOpenDocAEUPP);
		DisposeAEEventHandlerUPP(gHandlePrintDocAEUPP);
		DisposeAEEventHandlerUPP(gHandleQuitAppAEUPP);
#endif
	
	}
}


//////////
//
// QTApp_Idle
// Do any processing that can/should occur at idle time.
//
//////////

void QTApp_Idle (WindowReference theWindow)
{
	WindowObject 		myWindowObject = NULL;
	GrafPtr 			mySavedPort;
	//Cursor				myArrow;
	
	GetPort(&mySavedPort);
	MacSetPort(QTFrame_GetPortFromWindowReference(theWindow));
	
	myWindowObject = QTFrame_GetWindowObjectFromWindow(theWindow);
	if (myWindowObject != NULL) {
		MovieController		myMC = NULL;
	
		myMC = (**myWindowObject).fController;
		if (myMC != NULL) {
			
			// run any idle-time tasks for the movie
			
#if TARGET_OS_MAC
			// restore the cursor to the arrow
			// if it's outside the front movie window or outside the window's visible region
			if (theWindow == QTFrame_GetFrontMovieWindow()) {
				Rect			myRect;
				Point			myPoint;
				RgnHandle		myVisRegion;
				Cursor			myArrow;
				
				GetMouse(&myPoint);
				myVisRegion = NewRgn();
				GetPortVisibleRegion(QTFrame_GetPortFromWindowReference(theWindow), myVisRegion);
				GetWindowPortBounds(theWindow, &myRect);
				if (!MacPtInRect(myPoint, &myRect) || !PtInRgn(myPoint, myVisRegion))
					MacSetCursor(GetQDGlobalsArrow(&myArrow));
					
				DisposeRgn(myVisRegion);
			}
#endif // TARGET_OS_MAC
		}
	}
	
	// ***insert application-specific idle-time processing here***
	
	MacSetPort(mySavedPort);
}


//////////
//
// QTApp_Draw
// Update the non-movie controller parts of the specified window.
//
//////////

void QTApp_Draw (WindowReference theWindow)
{
	GrafPtr 			mySavedPort = NULL;
	GrafPtr 			myWindowPort = NULL;
	WindowPtr			myWindow = NULL;
	Rect				myRect;
	
	GetPort(&mySavedPort);
	myWindowPort = QTFrame_GetPortFromWindowReference(theWindow);
	myWindow = QTFrame_GetWindowFromWindowReference(theWindow);
	
	if (myWindowPort == NULL)
		return;
		
	MacSetPort(myWindowPort);
	
#if TARGET_API_MAC_CARBON
	GetPortBounds(myWindowPort, &myRect);
#else
	myRect = myWindowPort->portRect;
#endif

	BeginUpdate(myWindow);

	// update the main effects window only if we are not showing an effect;
	// otherwise, we will get a flash of the first source image if there is
	// an update while the effect is running
	
	// because the effect updates consecutive frames relatively quickly
	// while it is running, the effect drawing acts as an update scheme anyway
	if (!gCurrentState.fShowingEffect) {
		if (myWindow == gMainWindow)
			// draw the main effects window
			QTEffects_DrawEffectsWindow();
		else if (QTFrame_IsMovieWindow(theWindow)) {
			EraseRect(&myRect);
	
			// ***insert application-specific drawing here***
		}
	}
	
	EndUpdate(myWindow);
	MacSetPort(mySavedPort);
}

//////////
//
// QTApp_HandleContentClick
// Handle mouse button clicks in the specified window.
//
//////////

void QTApp_HandleContentClick (WindowReference theWindow, EventRecord *theEvent)
{
#pragma unused(theEvent)
	WindowPtr			myWindow = QTFrame_GetWindowFromWindowReference(theWindow);

	// clicking in the main effects window stops the effect
	if (myWindow == gMainWindow) {
		gCurrentState.fShowingEffect = false;
		gCurrentState.fTime = 1;
		QTEffects_DrawEffectsWindow();
	}
}


//////////
//
// QTApp_HandleKeyPress
// Handle application-specific key presses.
// Returns true if the key press was handled, false otherwise.
//
//////////

Boolean QTApp_HandleKeyPress (char theCharCode)
{
	Boolean		isHandled = true;
	
	switch (theCharCode) {
	
		// ***insert application-specific key-press processing here***

		default:
			isHandled = false;
			break;
	}

	return(isHandled);
}


//////////
//
// QTApp_HandleMenu
// Handle selections in the application's menus.
//
// The theMenuItem parameter is a UInt16 version of the Windows "menu item identifier". 
// When called from Windows, theMenuItem is simply the menu item identifier passed to the window procedure.
// When called from MacOS, theMenuItem is constructed like this:
// 	*high-order 8 bits == the Macintosh menu ID (1 thru 256)
// 	*low-order 8 bits == the Macintosh menu item (sequential from 1 to ordinal of last menu item in menu)
// In this way, we can simplify the menu-handling code. There are, however, some limitations, mainly that
// the menu item identifiers on Windows must be derived from the Mac values. 
//
//////////

Boolean QTApp_HandleMenu (UInt16 theMenuItem)
{
	Boolean				myIsHandled = false;			// false => allow caller to process the menu item
	OSErr				myErr = noErr;

	switch (theMenuItem) {
		case IDM_SELECT_EFFECT:					// let the user select an effect
		
			// stop the current effect (if any) from playing
			gCurrentState.fShowingEffect = false;
			gCurrentState.fTime = 0;
			QTEffects_DrawEffectsWindow();
			
			// let the user select an effect
			myErr = QTEffects_LetUserChooseEffect();
			if (myErr != noErr)
				break;

			// if the sample description is already allocated, deallocate it
			if (gCurrentState.fSampleDescription != NULL)
				DisposeHandle((Handle)gCurrentState.fSampleDescription);
				
			// create a sample description for the effect
			gCurrentState.fSampleDescription = QTEffects_MakeSampleDescription(gCurrentState.fEffectType, kWidth, kHeight);
			if (gCurrentState.fSampleDescription == NULL)
				break;
			
			// if the effect description is already allocated, dispose of it
			if (gCurrentState.fEffectDescription != NULL)
				QTDisposeAtomContainer(gCurrentState.fEffectDescription);
			
			// set up a new effect description
			gCurrentState.fEffectDescription = QTEffects_CreateEffectDescription(gCurrentState.fEffectType, kSourceOneName, kSourceTwoName);
			if (gCurrentState.fEffectDescription == NULL)
				break;
				
			// prompt the user to select options
			myErr = QTEffects_LetUserCustomizeEffect(gCurrentState.fEffectDescription);
			myIsHandled = true;
			break;

		case IDM_RUN_EFFECT:
			gCurrentState.fShowingEffect = true;
			gCurrentState.fSteppingEffect = false;
			myIsHandled = true;
			break;

		case IDM_STEP_AHEAD:
			gCurrentState.fShowingEffect = true;
			gCurrentState.fSteppingEffect = true;
			gCurrentDir = kForward;
			gFastEffectDisplay = false;
			myIsHandled = true;
			break;

		case IDM_STEP_BACK:
			gCurrentState.fShowingEffect = true;
			gCurrentState.fSteppingEffect = true;
			gCurrentDir = kBackward;
			gFastEffectDisplay = false;
			myIsHandled = true;
			break;

		case IDM_MAKE_EFFECT_MOVIE:
			QTEffects_CreateEffectsMovie(gCurrentState.fEffectType, gCurrentState.fEffectDescription, kWidth, kHeight);
			myIsHandled = true;
			break;

		case IDM_GET_FIRST_PICTURE:
			myErr = QTEffects_GetPictureAsGWorld(kWidth, kHeight, kDepth, &gGW1);
			if (myErr == noErr) {
			
				// we need to refresh image descriptions, etc.
				LockPixels(GetGWorldPixMap(gGW1));
				QTEffects_SetUpEffectSequence();
				QTEffects_DrawEffectsWindow();
			}
			myIsHandled = true;
			break;
			
		case IDM_GET_SECOND_PICTURE:
			myErr = QTEffects_GetPictureAsGWorld(kWidth, kHeight, kDepth, &gGW2);
			if (myErr == noErr) {
				// we need to refresh image descriptions, etc.
				LockPixels(GetGWorldPixMap(gGW2));
				QTEffects_SetUpEffectSequence();
				QTEffects_DrawEffectsWindow();
			}
			myIsHandled = true;
			break;
			
		case IDM_NO_LOOPING:
		case IDM_NORMAL_LOOPING:
		case IDM_PALINDROME_LOOPING:
			gLoopingState = theMenuItem;
			gFastEffectDisplay = (theMenuItem == IDM_NO_LOOPING);
			myIsHandled = true;
			break;
			
		case IDM_STANDARD_DIALOG:
		case IDM_CUSTOM_DIALOG:
			gUseStandardDialog = (theMenuItem == IDM_STANDARD_DIALOG);
			myIsHandled = true;
			break;
			
		case IDM_FAST_DISPLAY:
			gFastEffectDisplay = !gFastEffectDisplay;
			gLoopingState = kNoLooping;
			myIsHandled = true;
			break;

		default:
			break;
	} // switch (theMenuItem)
	
	return(myIsHandled);
}


//////////
//
// QTApp_AdjustMenus
// Adjust state of items in the application's menus.
//
// Currently, the Mac application has only one app-specific menu ("Test"); you could change that.
//
//////////

void QTApp_AdjustMenus (WindowReference theWindow, MenuReference theMenu)
{
#pragma unused(theWindow)
	MenuReference			myMenu;
	
#if TARGET_OS_WIN32
	myMenu = theMenu;
#endif

	// adjust the Settings menu
#if TARGET_OS_MAC
#pragma unused(theMenu)
	myMenu = GetMenuHandle(kSettingsMenuResID);
#endif

	QTFrame_SetMenuItemCheck(myMenu, IDM_NO_LOOPING, (gLoopingState == kNoLooping));
	QTFrame_SetMenuItemCheck(myMenu, IDM_NORMAL_LOOPING, (gLoopingState == kNormalLooping));
	QTFrame_SetMenuItemCheck(myMenu, IDM_PALINDROME_LOOPING, (gLoopingState == kPalindromeLooping));
	QTFrame_SetMenuItemCheck(myMenu, IDM_STANDARD_DIALOG, (gUseStandardDialog == true));
	QTFrame_SetMenuItemCheck(myMenu, IDM_CUSTOM_DIALOG, (gUseStandardDialog == false));
	QTFrame_SetMenuItemCheck(myMenu, IDM_FAST_DISPLAY, (gFastEffectDisplay == true));

	// now, do all Effects menu adjustment
#if TARGET_OS_MAC
	myMenu = GetMenuHandle(kEffectMenuResID);
#endif
}


//////////
//
// QTApp_HandleEvent
// Perform any application-specific event loop actions.
//
// Return true to indicate that we've completely handled the event here, false otherwise.
//
//////////

Boolean QTApp_HandleEvent (EventRecord *theEvent)
{
	Boolean		isHandled = false;
	
	// run the next step(s) of the effect
	QTEffects_ProcessEffect();
	
	// see if the event is meant for the effects parameter dialog box
	if (gEffectsDialog != 0L)
		isHandled = QTEffects_HandleEffectsDialogEvents(theEvent, 0);

#if TARGET_OS_MAC
	// handle events for any application-specific windows
	if (!isHandled)
		isHandled = QTEffects_HandleEffectsWindowEvents(theEvent);
#endif

	return(isHandled);
}


//////////
//
// QTApp_SetupController
// Configure the movie controller.
//
//////////

void QTApp_SetupController (MovieController theMC)
{
	long			myControllerFlags;
	
	// CLUT table use
	MCDoAction(theMC, mcActionGetFlags, &myControllerFlags);
	MCDoAction(theMC, mcActionSetFlags, (void *)(myControllerFlags | mcFlagsUseWindowPalette));

	// enable keyboard event handling
	MCDoAction(theMC, mcActionSetKeysEnabled, (void *)true);
	
	// disable drag support
	MCDoAction(theMC, mcActionSetDragEnabled, (void *)false);
}


//////////
//
// QTApp_SetupWindowObject
// Do any application-specific initialization of the window object.
//
//////////

void QTApp_SetupWindowObject (WindowObject theWindowObject)
{
#pragma unused(theWindowObject)

	// ***insert application-specific window object configuration here***
}


//////////
//
// QTApp_RemoveWindowObject
// Do any application-specific clean-up of the window object.
//
//////////

void QTApp_RemoveWindowObject (WindowObject theWindowObject)
{
#pragma unused(theWindowObject)
	
	// ***insert application-specific window object clean-up here***

	// QTFrame_DestroyMovieWindow in MacFramework.c or QTFrame_MovieWndProc in WinFramework.c
	// releases the window object itself
}


//////////
//
// QTApp_MCActionFilterProc 
// Intercept some actions for the movie controller.
//
// NOTE: The theRefCon parameter is a handle to a window object record.
//
//////////

PASCAL_RTN Boolean QTApp_MCActionFilterProc (MovieController theMC, short theAction, void *theParams, long theRefCon)
{
#pragma unused(theMC, theParams)

	Boolean				isHandled = false;			// false => allow controller to process the action
	WindowObject		myWindowObject = NULL;
	
	myWindowObject = (WindowObject)theRefCon;
	if (myWindowObject == NULL)
		return(isHandled);
		
	switch (theAction) {
	
		// handle window resizing
		case mcActionControllerSizeChanged:
			if (MCIsControllerAttached(theMC) == 1)
				QTFrame_SizeWindowToMovie(myWindowObject);
			break;

		// handle idle events
		case mcActionIdle:
			QTApp_Idle((**myWindowObject).fWindow);
			break;
			
		default:
			break;
			
	} // switch (theAction)
	
	return(isHandled);	
}


#if TARGET_OS_MAC
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Apple Event functions.
//
// Use these functions to install handlers for Apple Events and to handle those events.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////
//
// QTApp_InstallAppleEventHandlers 
// Install handlers for Apple Events.
//
//////////

void QTApp_InstallAppleEventHandlers (void)
{
	long		myAttrs;
	OSErr		myErr = noErr;
	
	// see whether the Apple Event Manager is available in the present operating environment;
	// if it is, install handlers for the four required Apple Events
	myErr = Gestalt(gestaltAppleEventsAttr, &myAttrs);
	if (myErr == noErr) {
		if (myAttrs & (1L << gestaltAppleEventsPresent)) {
			// create routine descriptors for the Apple event handlers
			gHandleOpenAppAEUPP = NewAEEventHandlerUPP(QTApp_HandleOpenApplicationAppleEvent);
			gHandleOpenDocAEUPP = NewAEEventHandlerUPP(QTApp_HandleOpenDocumentAppleEvent);
			gHandlePrintDocAEUPP = NewAEEventHandlerUPP(QTApp_HandlePrintDocumentAppleEvent);
			gHandleQuitAppAEUPP = NewAEEventHandlerUPP(QTApp_HandleQuitApplicationAppleEvent);
			
			// install the handlers
			AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, gHandleOpenAppAEUPP, 0L, false);
			AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, gHandleOpenDocAEUPP, 0L, false);
			AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments, gHandlePrintDocAEUPP, 0L, false);
			AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, gHandleQuitAppAEUPP, 0L, false);
		}
	}
}
		
		
//////////
//
// QTApp_HandleOpenApplicationAppleEvent 
// Handle the open-application Apple Events.
//
//////////

PASCAL_RTN OSErr QTApp_HandleOpenApplicationAppleEvent (const AppleEvent *theMessage, AppleEvent *theReply, long theRefcon)			
{
#pragma unused(theMessage, theReply, theRefcon)
	
	// we don't need to do anything special when opening the application
	return(noErr);
}


//////////
//
// QTApp_HandleOpenDocumentAppleEvent 
// Handle the open-document Apple Events. This is based on Inside Macintosh: IAC, pp. 4-15f.
//
// Here we process an Open Documents AE only for files of type MovieFileType.
//
//////////

PASCAL_RTN OSErr QTApp_HandleOpenDocumentAppleEvent (const AppleEvent *theMessage, AppleEvent *theReply, long theRefcon)			
{
#pragma unused(theReply, theRefcon)

	long			myIndex;
	long			myItemsInList;
	AEKeyword		myKeyWd;
	AEDescList 	 	myDocList;
	long			myActualSize;
	DescType		myTypeCode;
	FSSpec			myFSSpec;
	OSErr			myIgnoreErr = noErr;
	OSErr			myErr = noErr;
	
	// get the direct parameter and put it into myDocList
	myDocList.dataHandle = NULL;
	myErr = AEGetParamDesc(theMessage, keyDirectObject, typeAEList, &myDocList);
	
	// count the descriptor records in the list
	if (myErr == noErr)
		myErr = AECountItems(&myDocList, &myItemsInList);
	else
		myItemsInList = 0;
	
	// open each specified file
	for (myIndex = 1; myIndex <= myItemsInList; myIndex++)
		if (myErr == noErr) {
			myErr = AEGetNthPtr(&myDocList, myIndex, typeFSS, &myKeyWd, &myTypeCode, (Ptr)&myFSSpec, sizeof(myFSSpec), &myActualSize);
			if (myErr == noErr) {
				FInfo		myFinderInfo;
			
				// verify that the file type is MovieFileType; to do this, get the Finder information
				myErr = FSpGetFInfo(&myFSSpec, &myFinderInfo);	
				if (myErr == noErr) {
					if (myFinderInfo.fdType == MovieFileType)
						// we've got a movie file; just open it
						QTFrame_OpenMovieInWindow(NULL, &myFSSpec);
				}
			}
		}

	if (myDocList.dataHandle)
		myIgnoreErr = AEDisposeDesc(&myDocList);
	
	// make sure we open the document in the foreground		
	gAppInForeground = true;
	return(myErr);
}


//////////
//
// QTApp_HandlePrintDocumentAppleEvent 
// Handle the print-document Apple Events.
//
// NOT YET IMPLEMENTED.
//
//////////

PASCAL_RTN OSErr QTApp_HandlePrintDocumentAppleEvent (const AppleEvent *theMessage, AppleEvent *theReply, long theRefcon)			
{
#pragma unused(theMessage, theReply, theRefcon)

	return(errAEEventNotHandled);
}


//////////
//
// QTApp_HandleQuitApplicationAppleEvent 
// Handle the quit-application Apple Events.
//
//////////

PASCAL_RTN OSErr QTApp_HandleQuitApplicationAppleEvent (const AppleEvent *theMessage, AppleEvent *theReply, long theRefcon)			
{
#pragma unused(theMessage, theReply, theRefcon)

	// close down the entire framework and application
	QTFrame_QuitFramework();
	return(noErr);
}
#endif // TARGET_OS_MAC


