#include "bt_wasm_xm.h"

#include <emscripten/emscripten.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>

struct BTWasmWidget {
  const char *name;
  Widget parent;
  int x;
  int y;
  int width;
  int height;
  void (*activate)();
  void *activate_data;
  void (*expose)();
  void *expose_data;
};

struct BTWasmDisplay {};
struct BTWasmVisual {};
struct BTWasmScreen {};
struct BTWasmApp {};
struct BTWasmWidgetClass { const char *name; };

static BTWasmDisplay display_;
static BTWasmVisual visual_;
static BTWasmScreen screen_;
static BTWasmApp app_;
static BTWasmWidgetClass shell_class_ = {"shell"};
static BTWasmWidgetClass drawing_class_ = {"drawing"};
static BTWasmWidgetClass form_class_ = {"form"};
static BTWasmWidgetClass label_class_ = {"label"};
static BTWasmWidgetClass message_class_ = {"message"};
static BTWasmWidgetClass push_class_ = {"push"};
static BTWasmWidgetClass row_class_ = {"row"};
static BTWasmWidgetClass scale_class_ = {"scale"};

static char wasm_name_[] = "wasm";
static char wasm_gecos_[] = "BattleTris Player";
static char wasm_dir_[] = "/";
static char wasm_shell_[] = "/bin/sh";
static passwd wasm_passwd_ = { wasm_name_, wasm_name_, 1000, 1000, wasm_gecos_, wasm_dir_, wasm_shell_ };

passwd *bt_wasm_getpwuid(uid_t) { return &wasm_passwd_; }
passwd *bt_wasm_getpwnam(const char *) { return &wasm_passwd_; }
static BTWasmWidgetClass list_class_ = {"list"};
static BTWasmWidgetClass text_class_ = {"text"};
static BTWasmWidgetClass toggle_class_ = {"toggle"};
static BTWasmWidgetClass frame_class_ = {"frame"};

WidgetClass applicationShellWidgetClass = &shell_class_;
WidgetClass xmDrawingAreaWidgetClass = &drawing_class_;
WidgetClass xmFormWidgetClass = &form_class_;
WidgetClass xmLabelWidgetClass = &label_class_;
WidgetClass xmMessageBoxWidgetClass = &message_class_;
WidgetClass xmPushButtonWidgetClass = &push_class_;
WidgetClass xmRowColumnWidgetClass = &row_class_;
WidgetClass xmScaleWidgetClass = &scale_class_;
WidgetClass xmScrolledListWidgetClass = &list_class_;
WidgetClass xmScrolledTextWidgetClass = &text_class_;
WidgetClass xmTextWidgetClass = &text_class_;
WidgetClass xmTextFieldWidgetClass = &text_class_;
WidgetClass xmToggleButtonGadgetClass = &toggle_class_;
WidgetClass xmFrameWidgetClass = &frame_class_;

static Widget make_widget(const char *name, Widget parent) {
  Widget w = (Widget)calloc(1, sizeof(*w));
  w->name = name;
  w->parent = parent;
  w->width = 100;
  w->height = 40;
  return w;
}

static void fire_expose(void *arg) {
  Widget w = (Widget)arg;
  if (w && w->expose)
    ((void (*)(Widget, XtPointer, XtPointer))w->expose)(w, w->expose_data, 0);
}

void XtToolkitInitialize(void) {}
XtAppContext XtCreateApplicationContext(void) { return &app_; }
void XtDestroyApplicationContext(XtAppContext) {}
void XtAppSetFallbackResources(XtAppContext, char **) {}
void XtAppSetWarningHandler(XtAppContext, void (*)(String)) {}
void XtAppSetErrorHandler(XtAppContext, void (*)(String)) {}
Display *XtOpenDisplay(XtAppContext, const char *, const char *, const char *, XrmOptionDescRec *, unsigned int, int *, char **) { return &display_; }
Display *XtOpenDisplay(XtAppContext c, const char *a, const char *b, const char *d, XrmOptionDescRec *o, unsigned int n, unsigned int *argc, char **argv) {
  return XtOpenDisplay(c, a, b, d, o, n, (int *)argc, argv);
}
void XtCloseDisplay(Display *) {}
Widget XtAppCreateShell(const char *name, const char *, WidgetClass, Display *, Arg *, unsigned int) { return make_widget(name, 0); }
Widget XtVaCreateWidget(const char *name, WidgetClass, Widget parent, ...) { return make_widget(name, parent); }
Widget XtVaCreateManagedWidget(const char *name, WidgetClass c, Widget parent, ...) { Widget w = make_widget(name, parent); XtManageChild(w); return w; }
Widget XmCreateDialogShell(Widget parent, char *name, Arg *, unsigned int) { return make_widget(name, parent); }
Widget XmCreateWarningDialog(Widget parent, char *name, Arg *, unsigned int) { return make_widget(name, parent); }
Widget XmCreateScrolledList(Widget parent, char *name, Arg *, unsigned int) { return make_widget(name, parent); }
Widget XmCreateScrolledText(Widget parent, char *name, Arg *, unsigned int) { return make_widget(name, parent); }
void XtManageChild(Widget w) { if (w && w->expose) emscripten_async_call(fire_expose, w, 0); }
void XtUnmanageChild(Widget) {}
void XtDestroyWidget(Widget w) { free(w); }
void XtRealizeWidget(Widget w) { if (w && w->expose) emscripten_async_call(fire_expose, w, 0); }
void XtRemoveCallback(Widget, const char *, void (*)(), XtPointer) {}
void XtSetArg(Arg, const char *, long) {}
Widget XtParent(Widget w) { return w ? w->parent : 0; }
void XtFree(char *p) { free(p); }
void *XtMalloc(unsigned int n) { return malloc(n); }
XtTranslations XtParseTranslationTable(String) { return 0; }
void XtAppAddActions(XtAppContext, XtActionsRec *, unsigned int) {}
void XtOverrideTranslations(Widget, XtTranslations) {}
XtInputMask XtAppPending(XtAppContext) { return 0; }
void XtAppProcessEvent(XtAppContext, XtInputMask) {}
void XtAppMainLoop(XtAppContext) { emscripten_exit_with_live_runtime(); }
void XtAddRawEventHandler(Widget, long, Boolean, void (*)(), XtPointer) {}
void XtRemoveRawEventHandler(Widget, long, Boolean, void (*)(), XtPointer) {}
void XtAddEventHandler(Widget, long, Boolean, void (*)(), XtPointer) {}
XtInputId XtAppAddInput(XtAppContext, int, XtPointer, void (*)(), XtPointer) { return 1; }
void XtRemoveInput(XtInputId) {}
void XtGetApplicationResources(Widget, void *base, XtResource *res, unsigned int n, void *, unsigned int) {
  for (unsigned int i = 0; i < n; i++)
    memcpy((char *)base + res[i].resource_offset, &res[i].default_addr, res[i].resource_size < sizeof(void *) ? res[i].resource_size : sizeof(void *));
}
void XtRemoveTimeOut(XtIntervalId) {}
Display *XtDisplay(Widget) { return &display_; }
Window XtWindow(Widget w) { return (Window)(unsigned long)w; }
Screen *XtScreen(Widget) { return &screen_; }

static void timeout_tramp(void *arg) {
  void **a = (void **)arg;
  void (*cb)(XtPointer, XtIntervalId *) = (void (*)(XtPointer, XtIntervalId *))a[0];
  void *data = a[1];
  free(a);
  cb(data, 0);
}

XtIntervalId XtAppAddTimeOut(XtAppContext, unsigned long ms, void (*cb)(), XtPointer data) {
  void **arg = (void **)malloc(sizeof(void *) * 2);
  arg[0] = (void *)cb;
  arg[1] = data;
  emscripten_async_call(timeout_tramp, arg, (int)ms);
  return (XtIntervalId)arg;
}

void XtAddCallback(Widget w, const char *name, void (*cb)(), XtPointer data) {
  if (!w)
    return;
  if (!strcmp(name, XmNactivateCallback) || !strcmp(name, XmNvalueChangedCallback)) {
    w->activate = cb;
    w->activate_data = data;
  }
  if (!strcmp(name, XmNexposeCallback)) {
    w->expose = cb;
    w->expose_data = data;
    emscripten_async_call(fire_expose, w, 0);
  }
}

void XtVaSetValues(Widget w, ...) {
  va_list ap;
  va_start(ap, w);
  for (;;) {
    const char *name = va_arg(ap, const char *);
    if (!name)
      break;
    long value = va_arg(ap, long);
    if (!strcmp(name, XmNwidth)) w->width = (int)value;
    if (!strcmp(name, XmNheight)) w->height = (int)value;
    if (!strcmp(name, XmNx)) w->x = (int)value;
    if (!strcmp(name, XmNy)) w->y = (int)value;
  }
  va_end(ap);
}
void XtSetValues(Widget, Arg *, unsigned int) {}

void XtVaGetValues(Widget w, ...) {
  va_list ap;
  va_start(ap, w);
  for (;;) {
    const char *name = va_arg(ap, const char *);
    if (!name)
      break;
    void *out = va_arg(ap, void *);
    if (!strcmp(name, XmNwidth)) *(Dimension *)out = w ? w->width : 100;
    else if (!strcmp(name, XmNheight)) *(Dimension *)out = w ? w->height : 40;
    else if (!strcmp(name, XmNcolormap)) *(Colormap *)out = 0;
    else if (!strcmp(name, XmNvisual)) *(Visual **)out = &visual_;
    else if (!strcmp(name, XmNdepth)) *(int *)out = 24;
    else *(long *)out = 0;
  }
  va_end(ap);
}

XmString XmStringCreateSimple(char *s) { return s; }
XmString XmStringCreateLtoR(char *s, char *) { return s; }
XmString XmStringConcat(XmString a, XmString) { return a; }
XmString XmStringSeparatorCreate(void) { return 0; }
void XmStringFree(void *) {}
Boolean XmStringGetLtoR(void *s, char *, char **out) { *out = s ? strdup((char *)s) : 0; return True; }
Atom XmInternAtom(Display *, char *, Boolean) { return 1; }
void XmAddWMProtocolCallback(Widget, Atom, void (*)(), caddr_t) {}
Widget XmMessageBoxGetChild(Widget w, int) { return w; }

int XSetErrorHandler(int (*)(Display *, XErrorEvent *)) { return 0; }
int XSetIOErrorHandler(int (*)(Display *)) { return 0; }
int DefaultScreen(Display *) { return 0; }
Visual *DefaultVisual(Display *, int) { return &visual_; }
Window RootWindow(Display *, int) { return 1; }
int DefaultDepth(Display *, int) { return 24; }
Colormap XDefaultColormap(Display *, int) { return 0; }
GC XDefaultGC(Display *, int) { return 1; }
Pixmap XCreatePixmap(Display *, Window, unsigned int, unsigned int, unsigned int) { static Pixmap p = 10; return p++; }
Colormap XCreateColormap(Display *, Window, Visual *, int) { return 0; }
Colormap XCopyColormapAndFree(Display *, Colormap c) { return c; }
Window XCreateWindow(Display *, Window, int, int, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual *, unsigned long, void *) { static Window w = 100; return w++; }
GC XCreateGC(Display *, Window, unsigned long, XGCValues *) { static GC g = 20; return g++; }
void XFreeGC(Display *, GC) {}
void XFreeColormap(Display *, Colormap) {}
void XFreePixmap(Display *, Pixmap) {}
void XFree(void *p) { free(p); }
void XDestroyWindow(Display *, Window) {}
void XFlush(Display *) { EM_ASM({ if (Module.btFlush) Module.btFlush(); }); }
void XSync(Display *, Boolean) {}
void XGetErrorText(Display *, int, char *buf, int len) { if (len > 0) strncpy(buf, "wasm x error", len); }
char *DisplayString(Display *) { return (char *)"wasm"; }
void XSetForeground(Display *, GC, unsigned long) {}
void XSetBackground(Display *, GC, unsigned long) {}
void XFillRectangle(Display *, unsigned long, GC, int x, int y, unsigned int w, unsigned int h) { EM_ASM({ if (Module.btFillRect) Module.btFillRect($0,$1,$2,$3); }, x, y, w, h); }
void XFillArc(Display *, unsigned long, GC, int x, int y, unsigned int w, unsigned int h, int, int) { EM_ASM({ if (Module.btFillRect) Module.btFillRect($0,$1,$2,$3); }, x, y, w, h); }
void XDrawArc(Display *, unsigned long, GC, int, int, unsigned int, unsigned int, int, int) {}
void XDrawPoint(Display *, unsigned long, GC, int x, int y) { EM_ASM({ if (Module.btFillRect) Module.btFillRect($0,$1,1,1); }, x, y); }
void XCopyArea(Display *, unsigned long, unsigned long, GC, int, int, unsigned int w, unsigned int h, int x, int y) { EM_ASM({ if (Module.btFillRect) Module.btFillRect($0,$1,$2,$3); }, x, y, w, h); }
XImage *XCreateImage(Display *, Visual *, unsigned int, int, int, char *, unsigned int w, unsigned int h, int, int) { XImage *img = (XImage *)calloc(1, sizeof(XImage)); img->width = w; img->height = h; img->bytes_per_line = w * 4; return img; }
void XDestroyImage(XImage *img) { if (img) { free(img->data); free(img); } }
int XPutImage(Display *, unsigned long, GC, XImage *, int, int, int, int, unsigned int, unsigned int) { return 0; }
int XBitmapPad(Display *) { return 32; }
int XAllocColor(Display *, Colormap, XColor *c) { static unsigned long p = 1; c->pixel = p++; return 1; }
void XPutPixel(XImage *, int, int, unsigned long) {}
int XLookupString(XKeyEvent *, char *buf, int len, KeySym *, XComposeStatus *) { if (len > 0) buf[0] = 0; return 0; }
unsigned long BlackPixel(Display *, int) { return 0; }
unsigned long WhitePixel(Display *, int) { return 0xffffff; }
unsigned long BlackPixelOfScreen(Screen *) { return 0; }
XVisualInfo *XGetVisualInfo(Display *, long, XVisualInfo *info, int *n) {
  XVisualInfo *out = (XVisualInfo *)calloc(1, sizeof(XVisualInfo));
  *out = *info;
  out->visual = &visual_;
  out->colormap_size = 256;
  if (n) *n = 1;
  return out;
}
void XMapWindow(Display *, Window) {}
void XUnmapWindow(Display *, Window) {}
Cursor XCreateFontCursor(Display *, unsigned int) { return 1; }
void XDefineCursor(Display *, Window, Cursor) {}
void XUndefineCursor(Display *, Window) {}
void XmProcessTraversal(Widget, int) {}
void XmListSelectPos(Widget, int, Boolean) {}
void XmListDeselectAllItems(Widget) {}
void XmListDeleteAllItems(Widget) {}
void XmListSelectItem(Widget, XmString, Boolean) {}
void XmTextSetString(Widget, char *) {}
void XmTextReplace(Widget, int, int, char *) {}

extern "C" EMSCRIPTEN_KEEPALIVE void bt_wasm_activate(Widget w) {
  if (w && w->activate)
    ((void (*)(Widget, XtPointer, XtPointer))w->activate)(w, w->activate_data, 0);
}
