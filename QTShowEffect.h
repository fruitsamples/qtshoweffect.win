//////////
//
//	File:		QTShowEffect.h
//
//	Contains:	Code to generate a QuickTime movie with a QuickTime video effect in it,
//				and to preview an effect on zero, one, or two pictures.
//
//	Written by:	Tim Monroe
//				Based on existing ShowEffect code written by Dan Crow,
//				which was adapted from Peter Hoddie's original DoEffect code.
//
//	Copyright:	� 1996-1998 by Apple Computer, Inc., all rights reserved.
//
//	Change History (most recent first):
//
//	   <1>	 	12/15/97	rtm		first file; integrated existing code with shell framework
//	   
//////////


//////////
//
// header files
//
//////////

#include "ComApplication.h"
#include <Controls.h>
#include <ControlDefinitions.h>
#include <Devices.h>
#include <Dialogs.h>
#include <DiskInit.h>
#include <FileTypesAndCreators.h>
#include <Fonts.h>
#include <ImageCodec.h>
#include <ImageCompression.h>
#include <Movies.h>
#include <QuickTimeComponents.h>
#include <Resources.h>
#include <Sound.h>
#include <stdio.h>
#include <string.h>
#include <TextUtils.h>
#include <ToolUtils.h>

#if TARGET_OS_MAC
#include "MacFramework.h"
#endif

#if TARGET_OS_WIN32
#include "WinFramework.h"
#endif


//////////
//
// compiler flags
//
//////////

#define USES_MAKE_IMAGE_DESC_FOR_EFFECT	1		// use MakeImageDescriptionForEffect (QT 4.0 and later)
#define ALLOW_COMPOUND_EFFECTS			0		// add a compound effect to the output effects movie?


//////////
//
// constants
//
//////////

// the maximum number of items we'll allow in a pop-up menu
#define kMaxNumPopupMenuItems			75

// resource IDs
#define kCustomDialogID					132
#define kSelectDialogID					133
#define kSplashDialogID					134

#define kPopUpMenuID					133

#define kWatchCursorResID				128

#define kFirstPICTResID					129
#define kSecondPICTResID				130

// dialog item IDs of items in the Select Effect dialog box
#define kSelectButtonOK					1
#define kSelectPopUpID					2

// dialog item IDs of items in the custom Customize Effect dialog box
#define kCustomPopUpID					1
#define kCustomStaticTextID				2
#define kCustomButtonOK					3
#define kCustomUserItemID				4

// default width, height, and bit depth of the main window
#define kWidth							360
#define kHeight							360
#define kThumbNailWidth					64
#define kThumbNailHeight				64
#define kDepth							0

// effects sources names
#define kSourceOneName					FOUR_CHAR_CODE('srcA')
#define kSourceTwoName					FOUR_CHAR_CODE('srcB')
#define kSourceThreeName				FOUR_CHAR_CODE('srcC')
#define kSourceNoneName					FOUR_CHAR_CODE('srcZ')

// parameters for QTGetEffectsList	
#define kNoMinNumSources				-1
#define kNoMaxNumSources				-1		

// looping states
#define kNoLooping						IDM_NO_LOOPING
#define kNormalLooping					IDM_NORMAL_LOOPING
#define kPalindromeLooping				IDM_PALINDROME_LOOPING

// play directions
#define kForward						1
#define kBackward						2

// miscellaneous constants
#define kOneSecond						600
#define kEffectMovieDuration			(5 * kOneSecond)
#define k30StepsCount					30
#define kWindowOffset					75

#define kSaveEffectMoviePrompt			"Save effect movie file as:"
#define kSaveEffectMovieFileName		"Effect.mov"

#define kEffectsWindowClassName			"Effects Window"
#define kEffectsWindowTitle				"Show Video Effect"


//////////
//
// data types
//
//////////

// a structure to hold information about the Select Effect pop-up menu
typedef struct {
	MenuHandle				fMenu;
	short					fLastChosen;
	short					fNumberOfItems;
	char					fMenuText[kMaxNumPopupMenuItems][255];
	OSType					fItemInfo[kMaxNumPopupMenuItems];
} PopUpMenuInformation;

// a structure to hold information about the current effect
typedef struct {
	OSType					fEffectType;
	ImageDescriptionHandle	fSampleDescription;
	Boolean					fShowingEffect;
	Boolean					fSteppingEffect;
	ImageSequence			fEffectSequenceID;
	QTAtomContainer			fEffectDescription;
	TimeBase				fTimeBase;
	TimeValue				fTime;
} StateInformation;


//////////
//
// function prototypes
//
//////////

OSErr						QTEffects_Init (void);
OSErr						QTEffects_Stop (void);

void						QTEffects_ProcessEffect (void);
void						QTEffects_DrawEffectsWindow (void);
#if TARGET_OS_MAC
Boolean						QTEffects_HandleEffectsWindowEvents (EventRecord *theEvent);
#elif TARGET_OS_WIN32
LRESULT CALLBACK			QTEffects_HandleEffectsWindowMessages (HWND theWnd, UINT theMessage, UINT wParam, LONG lParam);
BOOL						QTEffects_RegisterEffectsWindowClass (HANDLE hInstance);
#endif

OSErr						QTEffects_LetUserChooseEffect (void);
OSErr						QTEffects_LetUserCustomizeEffect (QTAtomContainer theEffectDesc);
# if TARGET_OS_WIN32
static void					QTEffects_EffectsDialogCallback (EventRecord *theEvent, DialogRef theDialog, DialogItemIndex theItemHit);
LRESULT CALLBACK			QTEffects_CustomDialogWndProc (HWND theWnd, UINT theMessage, UINT wParam, LONG lParam);
#endif
Boolean						QTEffects_HandleEffectsDialogEvents (EventRecord *theEvent, DialogItemIndex theItemHit);

OSErr						QTEffects_InitializePopUpMenu (PopUpMenuInformation *theMenuInfo);
OSErr						QTEffects_UninitializePopUpMenu (PopUpMenuInformation *theMenuInfo);
OSErr						QTEffects_AddItemToPopUpMenu (PopUpMenuInformation *theMenuInfo, char *theItemText, OSType theItemInfo);
OSErr						QTEffects_AddListOfEffects (void);

QTAtomContainer				QTEffects_CreateEffectDescription (OSType theEffectName, OSType theSourceName1, OSType theSourceName2);
OSErr						QTEffects_AddTrackReferenceToInputMap (QTAtomContainer theInputMap, Track theTrack, Track theSrcTrack, OSType theSrcName);
OSErr						QTEffects_SetUpEffectSequence (void);
ImageDescriptionHandle		QTEffects_MakeSampleDescription (OSType theEffectType, short theWidth, short theHeight);
OSErr						QTEffects_RunEffect (TimeValue theTime);

OSErr						QTEffects_GetPictResourceAsGWorld (short theResID, short theWidth, short theHeight, short theDepth, GWorldPtr *theGW);
OSErr						QTEffects_GetPictureAsGWorld (short theWidth, short theHeight, short theDepth, GWorldPtr *theGW);
OSErr						QTEffects_AddVideoTrackFromGWorld (Movie *theMovie, GWorldPtr theGW, Track *theSourceTrack, long theStartTime, short theWidth, short theHeight);

void						QTEffects_CreateEffectsMovie (OSType theEffectType, QTAtomContainer theEffectDesc, short theWidth, short theHeight);
void						QTEffects_NewCreateEffectsMovie (OSType theEffectType, QTAtomContainer theEffectDesc, short theWidth, short theHeight);
void						QTEffects_AddFilmNoiseToMovie (Movie theMovie, Track theSrcTrack);
