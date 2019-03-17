/* tcc.exe wic.c -lgdiplus
https://msdn.microsoft.com/en-us/library/windows/desktop/ms533837.aspx
*/

#include <stdio.h>
#include <stdlib.h>
#include <guiddef.h>
#include <windows.h>

//#include <gdiplus.h>
#define WINGDIPAPI __stdcall
#define GDIPCONST const

typedef void *DebugEventProc;
typedef void GpImage;

typedef enum GpStatus {
	Ok = 0,
	GenericError = 1,
	InvalidParameter = 2,
	OutOfMemory = 3,
	ObjectBusy = 4,
	InsufficientBuffer = 5,
	NotImplemented = 6,
	Win32Error = 7,
	WrongState = 8,
	Aborted = 9,
	FileNotFound = 10,
	ValueOverflow = 11,
	AccessDenied = 12,
	UnknownImageFormat = 13,
	FontFamilyNotFound = 14,
	FontStyleNotFound = 15,
	NotTrueTypeFont = 16,
	UnsupportedGdiplusVersion = 17,
	GdiplusNotInitialized = 18,
	PropertyNotFound = 19,
	PropertyNotSupported = 20,
	ProfileNotFound = 21
} GpStatus;

typedef struct ImageCodecInfo {
	CLSID Clsid;
	GUID FormatID;
	WCHAR *CodecName;
	WCHAR *DllName;
	WCHAR *FormatDescription;
	WCHAR *FilenameExtension;
	WCHAR *MimeType;
	DWORD Flags;
	DWORD Version;
	DWORD SigCount;
	DWORD SigSize;
	BYTE *SigPattern;
	BYTE *SigMask;
} ImageCodecInfo;

typedef struct EncoderParameter {
	GUID Guid;
	ULONG NumberOfValues;
	ULONG Type;
	VOID *Value;
} EncoderParameter;

typedef struct EncoderParameters {
	UINT Count;
	EncoderParameter Parameter[1];
} EncoderParameters;

typedef GpStatus (WINGDIPAPI *NotificationHookProc)(ULONG_PTR *token);
typedef VOID (WINGDIPAPI *NotificationUnhookProc)(ULONG_PTR token);

typedef struct GdiplusStartupInput {
	UINT32 GdiplusVersion;
	DebugEventProc DebugEventCallback;
	BOOL SuppressBackgroundThread;
	BOOL SuppressExternalCodecs;
} GdiplusStartupInput;

typedef struct GdiplusStartupOutput {
	NotificationHookProc NotificationHook;
	NotificationUnhookProc NotificationUnhook;
} GdiplusStartupOutput;

VOID* WINGDIPAPI GdipAlloc(size_t);
VOID WINGDIPAPI GdipFree(VOID*);
GpStatus WINGDIPAPI GdiplusStartup(ULONG_PTR*,GDIPCONST GdiplusStartupInput*,GdiplusStartupOutput*);
GpStatus WINGDIPAPI GdipGetImageEncodersSize(UINT*,UINT*);
GpStatus WINGDIPAPI GdipGetImageEncoders(UINT,UINT,ImageCodecInfo*);
GpStatus WINGDIPAPI GdipLoadImageFromFile(GDIPCONST WCHAR*,GpImage**);
GpStatus WINGDIPAPI GdipSaveImageToFile(GpImage*,GDIPCONST WCHAR*,GDIPCONST CLSID*,GDIPCONST EncoderParameters*);
VOID WINGDIPAPI GdiplusShutdown(ULONG_PTR);

int wmain(int argc, WCHAR **argv)
{
    ULONG_PTR           gdiplusToken;
    GdiplusStartupInput gdiplusStartupInput = {1, NULL, FALSE, TRUE}; //MUST!
    //
    UINT num, size, i;
    ImageCodecInfo *pImageCodecInfo;
    WCHAR *suffix;
    CLSID clsid;
    BOOL can_encode = 0;
    int ret = 0;
    //
    GpImage *GpImage;
    //
    if (argc != 3) {
        wprintf(L"wic: (Windows Imaging Component) Image Converter, @YX Hao, #20170220\n");
        wprintf(L"Usage: %s <in-file> <out-file>\n", argv[0]);
        wprintf(L"Formats: Same as mspaint!\n");
        return 1;
    }
    //
    suffix = wcsrchr(argv[2], L'.');
    if (!suffix) {
        wprintf(L"Can't get out-file suffix!\n", argv[0]);
        return 2;
    }
    _wcslwr(suffix);
    //
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Ok ) {
        wprintf(L"GdiplusStartup error!\n");
        return 3;
    }
    //get Clsid
    if (GdipGetImageEncodersSize(&num, &size) != Ok
        || (pImageCodecInfo = (ImageCodecInfo*)(GdipAlloc(size)),
            GdipGetImageEncoders(num, size, pImageCodecInfo) != Ok) ) {
        wprintf(L"Detecting Gdiplus Encoders error!\n");
        ret = 4;
        goto END;
    }
    for (i = 0; i < num; i++) {
        if ( wcsstr( _wcslwr(pImageCodecInfo[i].FilenameExtension), suffix ) ) {
            clsid = pImageCodecInfo[i].Clsid;
            can_encode = 1;
            break;
        }
    }
    GdipFree(pImageCodecInfo);
    //
    if (!can_encode) {
        wprintf(L"Unsupported output format!\n");
        ret = 5;
        goto END;
    }
    if (GdipLoadImageFromFile(argv[1], &GpImage) != Ok) {
        wprintf(L"Unsupported input format!\n");
        ret = 6;
        goto END;
    }
    if (GdipSaveImageToFile(GpImage, argv[2], &clsid, NULL) != Ok) {
        wprintf(L"Saving output failed!\n");
        ret = 7;
        goto END;
    }
    wprintf(L"Saved: '%s'\n", argv[2]);
    //
END:
    GdiplusShutdown(gdiplusToken);
    return ret;
}