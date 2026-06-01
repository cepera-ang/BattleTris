#ifndef BT_WASM_XM_H
#define BT_WASM_XM_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef struct BTWasmWidget *Widget;
typedef struct BTWasmDisplay Display;
typedef struct BTWasmVisual Visual;
typedef struct BTWasmScreen Screen;
typedef struct BTWasmApp *XtAppContext;
typedef struct BTWasmWidgetClass *WidgetClass;
typedef union BTWasmXEvent XEvent;
typedef unsigned long Window;
typedef unsigned long Pixmap;
typedef unsigned long Colormap;
typedef unsigned long GC;
typedef unsigned long Pixel;
typedef unsigned long Atom;
typedef unsigned long KeySym;
typedef unsigned long XtIntervalId;
typedef unsigned long XtInputId;
typedef unsigned long XtInputMask;
typedef unsigned long Cursor;
typedef unsigned int Dimension;
typedef int Position;
typedef int Boolean;
typedef char *String;
typedef void *XtPointer;
typedef void *XtTranslations;
typedef void *XmString;
typedef XmString *XmStringTable;
typedef void *XtResourceList;
typedef void *XrmDatabase;
typedef char *caddr_t;

typedef void (*XtActionProc)(Widget, XEvent *, char **, unsigned int *);
typedef void (*XtInputCallbackProc)(XtPointer, int *, XtInputId *);
typedef struct { const char *name; XtActionProc proc; } XtActionsRec;
typedef struct { const char *option; const char *specifier; int argKind; caddr_t value; } XrmOptionDescRec;
typedef struct { char *resource_name; char *resource_class; char *resource_type; unsigned int resource_size; unsigned int resource_offset; char *default_type; XtPointer default_addr; } XtResource;
typedef struct { const char *name; long value; } Arg;
typedef struct { int reason; } XmAnyCallbackStruct;
typedef struct { int reason; XEvent *event; } XmDrawingAreaCallbackStruct;
typedef struct { int set; } XmToggleButtonCallbackStruct;
typedef struct { int reason; char *value; int length; } XmTextVerifyCallbackStruct;
typedef struct { int reason; int value; } XmScaleCallbackStruct;
typedef struct { int reason; void *item; int item_length; char *item_string; int item_position; } XmListCallbackStruct;
typedef struct { unsigned long pixel; unsigned short red, green, blue; char flags; } XColor;
typedef struct { unsigned long foreground; } XGCValues;
typedef struct { int type; unsigned long serial; unsigned char error_code; unsigned char request_code; unsigned char minor_code; unsigned long resourceid; } XErrorEvent;
typedef struct { int type; unsigned int keycode; } XKeyEvent;
typedef struct { int type; } XAnyEvent;
union BTWasmXEvent { int type; XKeyEvent xkey; XAnyEvent xany; };
typedef struct { int dummy; } XComposeStatus;
typedef struct { int width, height, bytes_per_line; char *data; } XImage;
typedef struct { Visual *visual; int screen; int depth; int c_class; int colormap_size; } XVisualInfo;
typedef struct { unsigned long background_pixel; unsigned long border_pixel; Colormap colormap; } XSetWindowAttributes;

#define True 1
#define False 0
#define TRUE 1
#define FALSE 0
#define None 0
#define BadAlloc 11
#define InputOutput 1
#define AllocNone 0
#define ZPixmap 2
#define XtInputReadMask 1
#define XtInputWriteMask 2
#define XtInputExceptMask 4
#define XmUNSPECIFIED_PIXMAP 0
#define XmATTACH_POSITION 1
#define XmATTACH_FORM 2
#define XmATTACH_WIDGET 3
#define XmATTACH_NONE 4
#define XmWORK_AREA 1
#define XmPACK_COLUMN 1
#define XmVERTICAL 1
#define XmHORIZONTAL 2
#define XmALIGNMENT_BEGINNING 1
#define XmALIGNMENT_CENTER 2
#define XmALIGNMENT_END 3
#define XmSHADOW_ETCHED_IN 1
#define XmN_OF_MANY 1
#define XmCR_EXPOSE 1
#define XmCR_ACTIVATE 2
#define XmCR_VALUE_CHANGED 8
#define XmRString "String"
#define XtRImmediate "Immediate"
#define XtRBoolean "Boolean"
#define XtRString "String"
#define XtRInt "Int"
#define XtRPixel "Pixel"
#define XtRTranslationTable "TranslationTable"
#define XtCString "String"
#define XtCBoolean "Boolean"
#define XtCValue "Value"
#define XtCTranslations "Translations"
#define XtCColor "Color"
#define XtNumber(a) ((unsigned int)(sizeof(a) / sizeof((a)[0])))
#define XtOffsetOf(s, m) ((unsigned int)offsetof(s, m))
#define XrmoptionNoArg 0
#define XrmoptionSepArg 1
#define TrueColor 4
#define PseudoColor 3
#define VisualClassMask 1
#define VisualScreenMask 2
#define CWBackPixel 1
#define CWBorderPixel 2
#define CWColormap 4

#define XtNborderPixmap "borderPixmap"
#define XtNbackgroundPixmap "backgroundPixmap"
#define XtNcolormap "colormap"
#define XtNdepth "depth"
#define XtNvisual "visual"
#define XtNargc "argc"
#define XtNargv "argv"
#define XtNwidth "width"
#define XtNheight "height"
#define XtNx "x"
#define XtNy "y"
#define XtNtopAttachment "topAttachment"
#define XtNleftAttachment "leftAttachment"
#define XtNrightAttachment "rightAttachment"
#define XtNbottomAttachment "bottomAttachment"
#define XtNtopPosition "topPosition"
#define XtNleftPosition "leftPosition"
#define XtNrightPosition "rightPosition"
#define XtNbottomPosition "bottomPosition"
#define XtNmarginHeight "marginHeight"
#define XtNmarginWidth "marginWidth"
#define XtNshadowType "shadowType"
#define XmNwidth XtNwidth
#define XmNheight XtNheight
#define XmNx XtNx
#define XmNy XtNy
#define XmNtopAttachment XtNtopAttachment
#define XmNleftAttachment XtNleftAttachment
#define XmNrightAttachment XtNrightAttachment
#define XmNbottomAttachment XtNbottomAttachment
#define XmNtopPosition XtNtopPosition
#define XmNleftPosition XtNleftPosition
#define XmNrightPosition XtNrightPosition
#define XmNbottomPosition XtNbottomPosition
#define XmNmarginHeight XtNmarginHeight
#define XmNmarginWidth XtNmarginWidth
#define XmNshadowType XtNshadowType
#define XmNrowColumnType "rowColumnType"
#define XmNpacking "packing"
#define XmNorientation "orientation"
#define XmNnumColumns "numColumns"
#define XmNset "set"
#define XmNlabelString "labelString"
#define XmNindicatorType "indicatorType"
#define XmNvalueChangedCallback "valueChangedCallback"
#define XmNmapCallback "mapCallback"
#define XmNdialogType "dialogType"
#define XmNdialogStyle "dialogStyle"
#define XmDIALOG_WARNING 1
#define XmDIALOG_FULL_APPLICATION_MODAL 1
#define XmDIALOG_OK_BUTTON 1
#define XmDIALOG_SEPARATOR 2
#define XmDIALOG_SYMBOL_LABEL 3
#define XmDIALOG_HELP_BUTTON 4
#define XmDIALOG_CANCEL_BUTTON 5
#define XmNmessageString "messageString"
#define XmNdefaultButtonType "defaultButtonType"
#define XmNokCallback "okCallback"
#define XmNshowArrows "showArrows"
#define XmNsliderMark "sliderMark"
#define XmNONE 0
#define XmSTATIC 1
#define XmBOTTOM_RIGHT 1
#define XmAUTOMATIC 1
#define XmCONSTANT 1
#define XmBROWSE_SELECT 1
#define XmNscrollBarPlacement "scrollBarPlacement"
#define XmNscrollingPolicy "scrollingPolicy"
#define XmNlistSizePolicy "listSizePolicy"
#define XmNscrollVertical "scrollVertical"
#define XmNscrollHorizontal "scrollHorizontal"
#define XmNwordWrap "wordWrap"
#define XmNrows "rows"
#define XmNcolumns "columns"
#define XmNautoShowCursorPosition "autoShowCursorPosition"
#define XmMULTI_LINE_EDIT 1
#define XmNdefaultActionCallback "defaultActionCallback"
#define XmNactivateCallback "activateCallback"
#define XmNexposeCallback "exposeCallback"
#define XmNresizeCallback "resizeCallback"
#define XmNinputCallback "inputCallback"
#define XmNmotionVerifyCallback "motionVerifyCallback"
#define XmNalignment "alignment"
#define XmNfillOnArm "fillOnArm"
#define XmNbackground "background"
#define XmNforeground "foreground"
#define XmNfractionBase "fractionBase"
#define XmNresizePolicy "resizePolicy"
#define XmRESIZE_NONE 0
#define XtVaTypedArg "typedArg"
#define XmNcolormap XtNcolormap
#define XmNvisual XtNvisual
#define XmNdepth XtNdepth
#define XmNbackgroundPixmap XtNbackgroundPixmap
#define XmNborderPixmap XtNborderPixmap
#define XmNallowShellResize "allowShellResize"
#define XmNscrollBarDisplayPolicy "scrollBarDisplayPolicy"
#define XmNeditMode "editMode"
#define XmNeditable "editable"
#define XmNcursorPositionVisible "cursorPositionVisible"
#define XmNitems "items"
#define XmNitemCount "itemCount"
#define XmNvisibleItemCount "visibleItemCount"
#define XmNselectionPolicy "selectionPolicy"
#define XmNbrowseSelectionCallback "browseSelectionCallback"
#define XmNvalue "value"
#define XmNresize "resize"
#define XmNrecomputeSize "recomputeSize"
#define XmNleftOffset "leftOffset"
#define XmNrightOffset "rightOffset"
#define XmNtopOffset "topOffset"
#define XmNbottomOffset "bottomOffset"
#define XmNchildType "childType"
#define XmNtopWidget "topWidget"
#define XmNleftWidget "leftWidget"
#define XmNrightWidget "rightWidget"
#define XmNbottomWidget "bottomWidget"
#define XmFRAME_TITLE_CHILD 1
#define XmTRAVERSE_CURRENT 1
#define XmFONTLIST_DEFAULT_TAG "default"
#define KeyPressMask 1
#define ButtonPressMask 2
#define ButtonRelease 5
#define GCForeground 1
#define DoRed 1
#define DoGreen 2
#define DoBlue 4
#define XmNminimum "minimum"
#define XmNmaximum "maximum"
#define XmNdecimalPoints "decimalPoints"
#define XmNshowValue "showValue"
#define XmNscaleMultiple "scaleMultiple"
#define XmNdragCallback "dragCallback"
#define XmNvalueChangedCallback "valueChangedCallback"

extern WidgetClass applicationShellWidgetClass;
extern WidgetClass xmDrawingAreaWidgetClass;
extern WidgetClass xmFormWidgetClass;
extern WidgetClass xmLabelWidgetClass;
extern WidgetClass xmMessageBoxWidgetClass;
extern WidgetClass xmPushButtonWidgetClass;
extern WidgetClass xmRowColumnWidgetClass;
extern WidgetClass xmScaleWidgetClass;
extern WidgetClass xmScrolledListWidgetClass;
extern WidgetClass xmScrolledTextWidgetClass;
extern WidgetClass xmTextWidgetClass;
extern WidgetClass xmTextFieldWidgetClass;
extern WidgetClass xmToggleButtonGadgetClass;
extern WidgetClass xmFrameWidgetClass;

void XtToolkitInitialize(void);
XtAppContext XtCreateApplicationContext(void);
void XtDestroyApplicationContext(XtAppContext);
void XtAppSetFallbackResources(XtAppContext, char **);
void XtAppSetWarningHandler(XtAppContext, void (*)(String));
void XtAppSetErrorHandler(XtAppContext, void (*)(String));
Display *XtOpenDisplay(XtAppContext, const char *, const char *, const char *, XrmOptionDescRec *, unsigned int, int *, char **);
Display *XtOpenDisplay(XtAppContext, const char *, const char *, const char *, XrmOptionDescRec *, unsigned int, unsigned int *, char **);
void XtCloseDisplay(Display *);
Widget XtAppCreateShell(const char *, const char *, WidgetClass, Display *, Arg *, unsigned int);
Widget XtVaCreateWidget(const char *, WidgetClass, Widget, ...);
Widget XtVaCreateManagedWidget(const char *, WidgetClass, Widget, ...);
Widget XmCreateDialogShell(Widget, char *, Arg *, unsigned int);
Widget XmCreateWarningDialog(Widget, char *, Arg *, unsigned int);
Widget XmCreateScrolledList(Widget, char *, Arg *, unsigned int);
Widget XmCreateScrolledText(Widget, char *, Arg *, unsigned int);
void XtManageChild(Widget);
void XtUnmanageChild(Widget);
void XtDestroyWidget(Widget);
void XtRealizeWidget(Widget);
void XtAddCallback(Widget, const char *, void (*)(), XtPointer);
template<class F> inline void XtAddCallback(Widget w, const char *n, F cb, XtPointer d) { XtAddCallback(w, n, (void (*)())cb, d); }
void XtRemoveCallback(Widget, const char *, void (*)(), XtPointer);
void XtVaSetValues(Widget, ...);
void XtSetValues(Widget, Arg *, unsigned int);
void XtVaGetValues(Widget, ...);
inline void XtSetArg(Arg &arg, const char *name, long val) {
  arg.name = name;
  arg.value = val;
}
template<class T> inline void XtSetArg(Arg &arg, const char *name, T *val) {
  arg.name = name;
  arg.value = (long)(uintptr_t)val;
}
Widget XtParent(Widget);
void XtFree(char *);
void *XtMalloc(unsigned int);
XtTranslations XtParseTranslationTable(String);
void XtAppAddActions(XtAppContext, XtActionsRec *, unsigned int);
void XtOverrideTranslations(Widget, XtTranslations);
XtInputMask XtAppPending(XtAppContext);
void XtAppProcessEvent(XtAppContext, XtInputMask);
void XtAddRawEventHandler(Widget, long, Boolean, void (*)(), XtPointer);
void XtRemoveRawEventHandler(Widget, long, Boolean, void (*)(), XtPointer);
template<class F> inline void XtAddRawEventHandler(Widget w, long m, Boolean b, F cb, XtPointer d) { XtAddRawEventHandler(w, m, b, (void (*)())cb, d); }
template<class F> inline void XtRemoveRawEventHandler(Widget w, long m, Boolean b, F cb, XtPointer d) { XtRemoveRawEventHandler(w, m, b, (void (*)())cb, d); }
void XtAddEventHandler(Widget, long, Boolean, void (*)(), XtPointer);
template<class F> inline void XtAddEventHandler(Widget w, long m, Boolean b, F cb, XtPointer d) { XtAddEventHandler(w, m, b, (void (*)())cb, d); }
XtInputId XtAppAddInput(XtAppContext, int, XtPointer, void (*)(), XtPointer);
template<class F> inline XtInputId XtAppAddInput(XtAppContext c, int fd, XtPointer m, F cb, XtPointer d) { return XtAppAddInput(c, fd, m, (void (*)())cb, d); }
void XtRemoveInput(XtInputId);
void XtGetApplicationResources(Widget, void *, XtResource *, unsigned int, void *, unsigned int);
XtIntervalId XtAppAddTimeOut(XtAppContext, unsigned long, void (*)(), XtPointer);
template<class F> inline XtIntervalId XtAppAddTimeOut(XtAppContext c, unsigned long ms, F cb, XtPointer d) { return XtAppAddTimeOut(c, ms, (void (*)())cb, d); }
void XtRemoveTimeOut(XtIntervalId);
void XtAppMainLoop(XtAppContext);
Display *XtDisplay(Widget);
Window XtWindow(Widget);
Screen *XtScreen(Widget);

XmString XmStringCreateSimple(char *);
XmString XmStringCreateLtoR(char *, char *);
XmString XmStringConcat(XmString, XmString);
XmString XmStringSeparatorCreate(void);
void XmStringFree(void *);
Atom XmInternAtom(Display *, char *, Boolean);
void XmAddWMProtocolCallback(Widget, Atom, void (*)(), caddr_t);
template<class F> inline void XmAddWMProtocolCallback(Widget w, Atom a, F cb, caddr_t d) { XmAddWMProtocolCallback(w, a, (void (*)())cb, d); }
Widget XmMessageBoxGetChild(Widget, int);

int XSetErrorHandler(int (*)(Display *, XErrorEvent *));
int XSetIOErrorHandler(int (*)(Display *));
int DefaultScreen(Display *);
Visual *DefaultVisual(Display *, int);
Window RootWindow(Display *, int);
int DefaultDepth(Display *, int);
Colormap XDefaultColormap(Display *, int);
#define DefaultColormap XDefaultColormap
GC XDefaultGC(Display *, int);
Pixmap XCreatePixmap(Display *, Window, unsigned int, unsigned int, unsigned int);
Colormap XCreateColormap(Display *, Window, Visual *, int);
Colormap XCopyColormapAndFree(Display *, Colormap);
Window XCreateWindow(Display *, Window, int, int, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual *, unsigned long, void *);
GC XCreateGC(Display *, Window, unsigned long, XGCValues *);
void XFreeGC(Display *, GC);
void XFreeColormap(Display *, Colormap);
void XFreePixmap(Display *, Pixmap);
void XFree(void *);
void XDestroyWindow(Display *, Window);
void XFlush(Display *);
void XSync(Display *, Boolean);
void XGetErrorText(Display *, int, char *, int);
char *DisplayString(Display *);
void XSetForeground(Display *, GC, unsigned long);
void XSetBackground(Display *, GC, unsigned long);
void XFillRectangle(Display *, unsigned long, GC, int, int, unsigned int, unsigned int);
void XFillArc(Display *, unsigned long, GC, int, int, unsigned int, unsigned int, int, int);
void XDrawArc(Display *, unsigned long, GC, int, int, unsigned int, unsigned int, int, int);
void XDrawPoint(Display *, unsigned long, GC, int, int);
void XCopyArea(Display *, unsigned long, unsigned long, GC, int, int, unsigned int, unsigned int, int, int);
XImage *XCreateImage(Display *, Visual *, unsigned int, int, int, char *, unsigned int, unsigned int, int, int);
void XDestroyImage(XImage *);
int XPutImage(Display *, unsigned long, GC, XImage *, int, int, int, int, unsigned int, unsigned int);
int XBitmapPad(Display *);
int XAllocColor(Display *, Colormap, XColor *);
void XPutPixel(XImage *, int, int, unsigned long);
int XLookupString(XKeyEvent *, char *, int, KeySym *, XComposeStatus *);
unsigned long BlackPixel(Display *, int);
unsigned long WhitePixel(Display *, int);
unsigned long BlackPixelOfScreen(Screen *);
XVisualInfo *XGetVisualInfo(Display *, long, XVisualInfo *, int *);
void XMapWindow(Display *, Window);
void XUnmapWindow(Display *, Window);
Cursor XCreateFontCursor(Display *, unsigned int);
void XDefineCursor(Display *, Window, Cursor);
void XUndefineCursor(Display *, Window);
void XmProcessTraversal(Widget, int);
void XmListSelectPos(Widget, int, Boolean);
void XmListDeselectAllItems(Widget);
void XmListDeleteAllItems(Widget);
void XmListSelectItem(Widget, XmString, Boolean);
void XmTextSetString(Widget, char *);
void XmTextReplace(Widget, int, int, char *);
Boolean XmStringGetLtoR(void *, char *, char **);

#endif
