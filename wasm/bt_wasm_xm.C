#include "bt_wasm_xm.h"

#include <emscripten/emscripten.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

struct BTWasmWidget {
  const char *name;
  WidgetClass widget_class;
  Widget parent;
  int x;
  int y;
  int width;
  int height;
  
  // Attachments
  int left_attachment;
  int right_attachment;
  int top_attachment;
  int bottom_attachment;
  int left_position;
  int right_position;
  int top_position;
  int bottom_position;
  Widget left_widget;
  Widget right_widget;
  Widget top_widget;
  Widget bottom_widget;
  int left_offset;
  int right_offset;
  int top_offset;
  int bottom_offset;
  
  // Form resources
  int fraction_base;
  
  // Widget content
  char *label_string;
  int set;          // for toggle buttons
  int slider_value; // for scale widgets
  int minimum;      // for scale widgets
  int maximum;      // for scale widgets
  int item_count;   // for list widgets
  int rows;
  int columns;
  
  
  // Callbacks
  void (*activate)(Widget, XtPointer, XtPointer);
  void *activate_data;
  void (*expose)(Widget, XtPointer, XtPointer);
  void *expose_data;
  void (*input_cb)(Widget, XtPointer, XtPointer);
  void *input_data;
  void (*kbd_handler)(Widget, XtPointer, XEvent *, Boolean *);
  void *kbd_data;
  
  bool managed;
  bool mapped;
};

struct BTWasmDisplay {};
struct BTWasmVisual {};
struct BTWasmScreen {};
struct BTWasmApp {};
struct BTWasmWidgetClass { const char *name; };
struct BTWasmGC {
  unsigned long foreground;
  unsigned long background;
};

struct BTWasmTimeout {
  void (*cb)(XtPointer, XtIntervalId *);
  XtPointer data;
  bool cancelled;
};
struct BTWasmInput {
  int fd;
  void (*cb)(XtPointer, int *, XtInputId *);
  XtPointer data;
  XtInputId id;
  bool removed;
};

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

// passwd shims
static char wasm_name_[] = "wasm";
static char wasm_gecos_[] = "BattleTris Player";
static char wasm_dir_[] = "/";
static char wasm_shell_[] = "/bin/sh";
static passwd wasm_passwd_ = { wasm_name_, wasm_name_, 1000, 1000, wasm_gecos_, wasm_dir_, wasm_shell_ };

passwd *bt_wasm_getpwuid(uid_t) { return &wasm_passwd_; }
passwd *bt_wasm_getpwnam(const char *) { return &wasm_passwd_; }

// keypress
static char last_key_char = 0;
static XtActionsRec wasm_actions_[32];
static unsigned int wasm_action_count_ = 0;
static Widget wasm_keyboard_widget_ = 0;
static int wasm_pending_timeouts_ = 0;
static BTWasmInput wasm_inputs_[128];
static int wasm_input_count_ = 0;
static int wasm_input_poll_started_ = 0;
extern "C" int bt_wasm_socket_ready(int fd);

static void set_widget_text(Widget w, const char *s, int len) {
  if (!w) return;
  if (w->label_string) free(w->label_string);
  if (!s) {
    w->label_string = NULL;
    return;
  }
  w->label_string = (char *)malloc(len + 1);
  memcpy(w->label_string, s, len);
  w->label_string[len] = '\0';
}

static Widget make_widget(const char *name, WidgetClass widget_class, Widget parent) {
  Widget w = (Widget)calloc(1, sizeof(*w));
  w->name = name ? name : "widget";
  w->widget_class = widget_class;
  w->parent = parent;
  w->width = 100;
  w->height = 40;
  w->managed = false;
  w->mapped = true;
  w->fraction_base = 100;
  w->rows = 0;
  w->columns = 0;
  w->left_attachment = XmATTACH_NONE;
  w->right_attachment = XmATTACH_NONE;
  w->top_attachment = XmATTACH_NONE;
  w->bottom_attachment = XmATTACH_NONE;
  
  EM_ASM({
    if (Module.createWidget) {
      Module.createWidget($0, UTF8ToString($1), UTF8ToString($2), $3);
    }
  }, (unsigned long)w, w->name, widget_class ? widget_class->name : "", (unsigned long)parent);
  
  return w;
}

static void update_widget_js(Widget w) {
  if (!w) return;
  // 1. Basic Geometry and State
  EM_ASM({
    if (Module.updateWidget) {
      Module.updateWidget($0, {
        x: $1, y: $2, width: $3, height: $4,
        managed: $5, mapped: $6,
        label: $7 ? UTF8ToString($7) : null,
        set: $8, slider_value: $9,
        hasKbdHandler: $10
      });
    }
  }, (unsigned long)w, w->x, w->y, w->width, w->height,
     w->managed ? 1 : 0, w->mapped ? 1 : 0,
     w->label_string, w->set, w->slider_value, w->kbd_handler ? 1 : 0);

  // 2. Attachments & Constraints
  EM_ASM({
    if (Module.updateWidget) {
      Module.updateWidget($0, {
        minimum: $1, maximum: $2, fraction_base: $3,
        left_attachment: $4, right_attachment: $5,
        top_attachment: $6, bottom_attachment: $7,
        left_position: $8, right_position: $9
      });
    }
  }, (unsigned long)w, w->minimum, w->maximum, w->fraction_base,
     w->left_attachment, w->right_attachment,
     w->top_attachment, w->bottom_attachment,
     w->left_position, w->right_position);

  // 3. Offsets & Target Widgets
  EM_ASM({
    if (Module.updateWidget) {
      Module.updateWidget($0, {
        top_position: $1, bottom_position: $2,
        left_offset: $3, right_offset: $4,
        top_offset: $5, bottom_offset: $6,
        left_widget: $7, right_widget: $8,
        top_widget: $9, bottom_widget: $10
      });
    }
  }, (unsigned long)w,
     w->top_position, w->bottom_position,
     w->left_offset, w->right_offset,
     w->top_offset, w->bottom_offset,
     (unsigned long)w->left_widget, (unsigned long)w->right_widget,
     (unsigned long)w->top_widget, (unsigned long)w->bottom_widget);
}

static void parse_args(Widget w, const char *attr, long val) {
  if (!w || !attr) return;
  if (!strcmp(attr, XmNwidth)) w->width = (int)val;
  else if (!strcmp(attr, XmNheight)) w->height = (int)val;
  else if (!strcmp(attr, XmNx)) w->x = (int)val;
  else if (!strcmp(attr, XmNy)) w->y = (int)val;
  else if (!strcmp(attr, XmNleftAttachment)) w->left_attachment = (int)val;
  else if (!strcmp(attr, XmNrightAttachment)) w->right_attachment = (int)val;
  else if (!strcmp(attr, XmNtopAttachment)) w->top_attachment = (int)val;
  else if (!strcmp(attr, XmNbottomAttachment)) w->bottom_attachment = (int)val;
  else if (!strcmp(attr, XmNleftPosition)) w->left_position = (int)val;
  else if (!strcmp(attr, XmNrightPosition)) w->right_position = (int)val;
  else if (!strcmp(attr, XmNtopPosition)) w->top_position = (int)val;
  else if (!strcmp(attr, XmNbottomPosition)) w->bottom_position = (int)val;
  else if (!strcmp(attr, XmNleftOffset)) w->left_offset = (int)val;
  else if (!strcmp(attr, XmNrightOffset)) w->right_offset = (int)val;
  else if (!strcmp(attr, XmNtopOffset)) w->top_offset = (int)val;
  else if (!strcmp(attr, XmNbottomOffset)) w->bottom_offset = (int)val;
  else if (!strcmp(attr, XmNleftWidget)) w->left_widget = (Widget)val;
  else if (!strcmp(attr, XmNrightWidget)) w->right_widget = (Widget)val;
  else if (!strcmp(attr, XmNtopWidget)) w->top_widget = (Widget)val;
  else if (!strcmp(attr, XmNbottomWidget)) w->bottom_widget = (Widget)val;
  else if (!strcmp(attr, XmNfractionBase)) w->fraction_base = (int)val;
  else if (!strcmp(attr, XmNrows)) w->rows = (int)val;
  else if (!strcmp(attr, XmNcolumns)) w->columns = (int)val;
  else if (!strcmp(attr, XmNlabelString)) {
    set_widget_text(w, (char *)val, val ? strlen((char *)val) : 0);
  }
  else if (!strcmp(attr, XmNset)) w->set = (int)val;
  else if (!strcmp(attr, XmNvalue)) {
    if (w->widget_class == xmTextWidgetClass || w->widget_class == xmTextFieldWidgetClass || w->widget_class == xmScrolledTextWidgetClass) {
      int len = val ? strlen((char *)val) : 0;
      if (val && w->rows > 0 && w->columns > 0) len = w->rows * w->columns;
      set_widget_text(w, (char *)val, len);
    } else {
      w->slider_value = (int)val;
    }
  }
  else if (!strcmp(attr, XmNminimum)) w->minimum = (int)val;
  else if (!strcmp(attr, XmNmaximum)) w->maximum = (int)val;
  else if (!strcmp(attr, XmNitemCount)) w->item_count = (int)val;
  else if (!strcmp(attr, XmNitems)) {
    if (w->label_string) {
      free(w->label_string);
      w->label_string = NULL;
    }
    if (w->item_count > 0 && val) {
      char **items = (char **)val;
      size_t total_len = 0;
      for (int i = 0; i < w->item_count; i++) {
        if (items[i]) {
          total_len += strlen(items[i]) + 1;
        }
      }
      if (total_len > 0) {
        w->label_string = (char *)malloc(total_len + 1);
        w->label_string[0] = '\0';
        char *ptr = w->label_string;
        for (int i = 0; i < w->item_count; i++) {
          if (items[i]) {
            size_t len = strlen(items[i]);
            memcpy(ptr, items[i], len);
            ptr += len;
            *ptr++ = '\n';
          }
        }
        if (ptr > w->label_string) {
          *(ptr - 1) = '\0';
        }
      }
    }
  }
}

static void fire_expose(void *arg) {
  Widget w = (Widget)arg;
  if (w && w->expose) {
    XmDrawingAreaCallbackStruct cbs;
    cbs.reason = XmCR_EXPOSE;
    cbs.event = 0;
    w->expose(w, w->expose_data, &cbs);
  }
}

static void timeout_tramp(void *arg) {
  BTWasmTimeout *t = (BTWasmTimeout *)arg;
  if (wasm_pending_timeouts_ > 0) wasm_pending_timeouts_--;
  if (!t->cancelled) {
    EM_ASM({ if (Module.btPhase) Module.btPhase("timeout"); });
    t->cb(t->data, (XtIntervalId *)t);
    EM_ASM({ if (Module.btPhase) Module.btPhase("idle"); });
  }
  free(t);
}
static void poll_inputs(void *) {
  for (int i = 0; i < wasm_input_count_; i++) {
    BTWasmInput *in = &wasm_inputs_[i];
    if (in->removed || !in->cb) continue;
    if (bt_wasm_socket_ready(in->fd)) in->cb(in->data, &in->fd, &in->id);
  }
  if (wasm_input_poll_started_) emscripten_async_call(poll_inputs, 0, 16);
}

void XtToolkitInitialize(void) {}
XtAppContext XtCreateApplicationContext(void) { return &app_; }
void XtDestroyApplicationContext(XtAppContext) {}
void XtAppSetFallbackResources(XtAppContext, char **) {}
void XtAppSetWarningHandler(XtAppContext, void (*)(String)) {}
void XtAppSetErrorHandler(XtAppContext, void (*)(String)) {}

struct BTWasmResource {
  const char *name;
  const char *value;
};
static BTWasmResource parsed_resources_[64];
static int num_parsed_resources_ = 0;

Display *XtOpenDisplay(XtAppContext, const char *, const char *, const char *, XrmOptionDescRec *options, unsigned int num_options, int *argc, char **argv) {
  if (argc && argv) {
    int i = 1;
    while (i < *argc) {
      bool matched = false;
      for (unsigned int j = 0; j < num_options; j++) {
        if (strcmp(argv[i], options[j].option) == 0) {
          matched = true;
          int remove_count = 1;
          const char *val = options[j].value;
          if (options[j].argKind == XrmoptionSepArg) {
            remove_count = 2;
            if (i + 1 < *argc) {
              val = argv[i + 1];
            }
          }
          
          if (num_parsed_resources_ < 64) {
            const char *res_name = options[j].specifier;
            if (res_name && res_name[0] == '.') {
              res_name++; // skip leading dot
            }
            parsed_resources_[num_parsed_resources_].name = res_name;
            parsed_resources_[num_parsed_resources_].value = val;
            num_parsed_resources_++;
          }
          
          if (i + remove_count <= *argc) {
            for (int k = i; k + remove_count < *argc; k++) {
              argv[k] = argv[k + remove_count];
            }
            *argc -= remove_count;
          } else {
            *argc = i;
          }
          break;
        }
      }
      if (!matched) {
        i++;
      }
    }
  }
  return &display_;
}
Display *XtOpenDisplay(XtAppContext c, const char *a, const char *b, const char *d, XrmOptionDescRec *o, unsigned int n, unsigned int *argc, char **argv) {
  return XtOpenDisplay(c, a, b, d, o, n, (int *)argc, argv);
}
void XtCloseDisplay(Display *) {}

Widget XtAppCreateShell(const char *name, const char *, WidgetClass widget_class, Display *, Arg *args, unsigned int n) {
  Widget w = make_widget(name, widget_class, 0);
  for (unsigned int i = 0; i < n; i++) {
    parse_args(w, args[i].name, args[i].value);
  }
  update_widget_js(w);
  return w;
}

Widget XtVaCreateWidget(const char *name, WidgetClass widget_class, Widget parent, ...) {
  Widget w = make_widget(name, widget_class, parent);
  va_list ap;
  va_start(ap, parent);
  for (;;) {
    const char *attr = va_arg(ap, const char *);
    if (!attr) break;
    if (!strcmp(attr, XtVaTypedArg)) {
      const char *rname = va_arg(ap, const char *);
      const char *rtype = va_arg(ap, const char *);
      long rval = va_arg(ap, long);
      int rsize = va_arg(ap, int);
      parse_args(w, rname, rval);
      continue;
    }
    long val = va_arg(ap, long);
    parse_args(w, attr, val);
  }
  va_end(ap);
  update_widget_js(w);
  return w;
}

Widget XtVaCreateManagedWidget(const char *name, WidgetClass widget_class, Widget parent, ...) {
  Widget w = make_widget(name, widget_class, parent);
  w->managed = true;
  va_list ap;
  va_start(ap, parent);
  for (;;) {
    const char *attr = va_arg(ap, const char *);
    if (!attr) break;
    if (!strcmp(attr, XtVaTypedArg)) {
      const char *rname = va_arg(ap, const char *);
      const char *rtype = va_arg(ap, const char *);
      long rval = va_arg(ap, long);
      int rsize = va_arg(ap, int);
      parse_args(w, rname, rval);
      continue;
    }
    long val = va_arg(ap, long);
    parse_args(w, attr, val);
  }
  va_end(ap);
  update_widget_js(w);
  return w;
}

Widget XmCreateDialogShell(Widget parent, char *name, Arg *args, unsigned int n) {
  Widget w = make_widget(name, applicationShellWidgetClass, parent);
  for (unsigned int i = 0; i < n; i++) {
    parse_args(w, args[i].name, args[i].value);
  }
  update_widget_js(w);
  return w;
}

Widget XmCreateWarningDialog(Widget parent, char *name, Arg *args, unsigned int n) {
  Widget w = make_widget(name, xmMessageBoxWidgetClass, parent);
  for (unsigned int i = 0; i < n; i++) {
    parse_args(w, args[i].name, args[i].value);
  }
  update_widget_js(w);
  return w;
}

Widget XmCreateScrolledList(Widget parent, char *name, Arg *args, unsigned int n) {
  Widget scrolled_window = make_widget("scrolled_window", xmFrameWidgetClass, parent);
  scrolled_window->managed = true;
  update_widget_js(scrolled_window);
  
  Widget list = make_widget(name, xmScrolledListWidgetClass, scrolled_window);
  list->managed = true;
  for (unsigned int i = 0; i < n; i++) {
    parse_args(list, args[i].name, args[i].value);
  }
  update_widget_js(list);
  return list;
}

Widget XmCreateScrolledText(Widget parent, char *name, Arg *args, unsigned int n) {
  Widget scrolled_window = make_widget("scrolled_window", xmFrameWidgetClass, parent);
  scrolled_window->managed = true;
  update_widget_js(scrolled_window);
  
  Widget text = make_widget(name, xmScrolledTextWidgetClass, scrolled_window);
  text->managed = true;
  for (unsigned int i = 0; i < n; i++) {
    parse_args(text, args[i].name, args[i].value);
  }
  update_widget_js(text);
  return text;
}

void XtManageChild(Widget w) {
  if (w) {
    w->managed = true;
    update_widget_js(w);
    if (w->expose) {
      emscripten_async_call(fire_expose, w, 0);
    }
  }
}

void XtUnmanageChild(Widget w) {
  if (w) {
    w->managed = false;
    update_widget_js(w);
  }
}

void XtDestroyWidget(Widget w) {
  if (w) {
    EM_ASM({
      if (Module.destroyWidget) {
        Module.destroyWidget($0);
      }
    }, (unsigned long)w);
    if (w->label_string) free(w->label_string);
    free(w);
  }
}

void XtRealizeWidget(Widget w) {
  if (w) {
    w->mapped = true;
    update_widget_js(w);
    EM_ASM({ if (Module.createDrawable) Module.createDrawable($0, $1, $2); }, (Window)w, w->width, w->height);
    if (w->expose) {
      emscripten_async_call(fire_expose, w, 0);
    }
  }
}

void XtAddCallback(Widget w, const char *name, void (*cb)(), XtPointer data) {
  if (!w) return;
  if (!strcmp(name, XmNactivateCallback) || !strcmp(name, XmNvalueChangedCallback) || !strcmp(name, XmNdragCallback)) {
    w->activate = (void (*)(Widget, XtPointer, XtPointer))cb;
    w->activate_data = data;
  }
  else if (!strcmp(name, XmNexposeCallback)) {
    w->expose = (void (*)(Widget, XtPointer, XtPointer))cb;
    w->expose_data = data;
    emscripten_async_call(fire_expose, w, 0);
  }
  else if (!strcmp(name, XmNinputCallback)) {
    w->input_cb = (void (*)(Widget, XtPointer, XtPointer))cb;
    w->input_data = data;
  }
}

void XtRemoveCallback(Widget w, const char *name, void (*cb)(), XtPointer data) {
  if (!w) return;
  if (!strcmp(name, XmNactivateCallback) || !strcmp(name, XmNvalueChangedCallback) || !strcmp(name, XmNdragCallback)) {
    w->activate = 0;
    w->activate_data = 0;
  }
  else if (!strcmp(name, XmNexposeCallback)) {
    w->expose = 0;
    w->expose_data = 0;
  }
  else if (!strcmp(name, XmNinputCallback)) {
    w->input_cb = 0;
    w->input_data = 0;
  }
}

void XtVaSetValues(Widget w, ...) {
  if (!w) return;
  va_list ap;
  va_start(ap, w);
  for (;;) {
    const char *attr = va_arg(ap, const char *);
    if (!attr) break;
    if (!strcmp(attr, XtVaTypedArg)) {
      const char *rname = va_arg(ap, const char *);
      const char *rtype = va_arg(ap, const char *);
      long rval = va_arg(ap, long);
      int rsize = va_arg(ap, int);
      parse_args(w, rname, rval);
      continue;
    }
    long val = va_arg(ap, long);
    parse_args(w, attr, val);
  }
  va_end(ap);
  update_widget_js(w);
}

void XtSetValues(Widget w, Arg *args, unsigned int n) {
  if (!w || !args) return;
  for (unsigned int i = 0; i < n; i++) {
    parse_args(w, args[i].name, args[i].value);
  }
  update_widget_js(w);
}

void XtVaGetValues(Widget w, ...) {
  if (!w) return;
  va_list ap;
  va_start(ap, w);
  for (;;) {
    const char *attr = va_arg(ap, const char *);
    if (!attr) break;
    void *out = va_arg(ap, void *);
    if (!strcmp(attr, XmNwidth)) *(Dimension *)out = w->width;
    else if (!strcmp(attr, XmNheight)) *(Dimension *)out = w->height;
    else if (!strcmp(attr, XmNcolormap)) *(Colormap *)out = 0;
    else if (!strcmp(attr, XmNvisual)) *(Visual **)out = &visual_;
    else if (!strcmp(attr, XmNdepth)) *(int *)out = 24;
    else if (!strcmp(attr, XmNvalue)) *(int *)out = w->slider_value;
    else if (!strcmp(attr, XmNset)) *(int *)out = w->set;
    else *(long *)out = 0;
  }
  va_end(ap);
}

Widget XtParent(Widget w) { return w ? w->parent : 0; }
void XtFree(char *p) { free(p); }
void *XtMalloc(unsigned int n) { return malloc(n); }
XtTranslations XtParseTranslationTable(String) { return 0; }
void XtAppAddActions(XtAppContext, XtActionsRec *actions, unsigned int n) {
  for (unsigned int i = 0; i < n && wasm_action_count_ < XtNumber(wasm_actions_); i++)
    wasm_actions_[wasm_action_count_++] = actions[i];
}
void XtOverrideTranslations(Widget, XtTranslations) {}
XtInputMask XtAppPending(XtAppContext) { return 0; }
void XtAppProcessEvent(XtAppContext, XtInputMask) {}
void XtAppMainLoop(XtAppContext) { emscripten_exit_with_live_runtime(); }

void XtAddRawEventHandler(Widget w, long mask, Boolean, void (*cb)(), XtPointer data) {
  if (w && (mask & KeyPressMask)) {
    w->kbd_handler = (void (*)(Widget, XtPointer, XEvent *, Boolean *))cb;
    w->kbd_data = data;
    wasm_keyboard_widget_ = w;
    update_widget_js(w);
  }
}
void XtRemoveRawEventHandler(Widget w, long mask, Boolean, void (*cb)(), XtPointer data) {
  if (w && (mask & KeyPressMask)) {
    w->kbd_handler = 0;
    w->kbd_data = 0;
    if (wasm_keyboard_widget_ == w) wasm_keyboard_widget_ = 0;
    update_widget_js(w);
  }
}
void XtAddEventHandler(Widget w, long mask, Boolean nonmaskable, void (*cb)(), XtPointer data) {
  XtAddRawEventHandler(w, mask, nonmaskable, cb, data);
}

XtInputId XtAppAddInput(XtAppContext, int fd, XtPointer, void (*cb)(), XtPointer data) {
  if (!wasm_input_poll_started_) {
    wasm_input_poll_started_ = 1;
    emscripten_async_call(poll_inputs, 0, 16);
  }
  if (wasm_input_count_ >= (int)XtNumber(wasm_inputs_)) return 0;
  XtInputId id = (XtInputId)(unsigned long)(wasm_input_count_ + 1);
  wasm_inputs_[wasm_input_count_].fd = fd;
  wasm_inputs_[wasm_input_count_].cb = (void (*)(XtPointer, int *, XtInputId *))cb;
  wasm_inputs_[wasm_input_count_].data = data;
  wasm_inputs_[wasm_input_count_].id = id;
  wasm_inputs_[wasm_input_count_].removed = false;
  wasm_input_count_++;
  return id;
}
void XtRemoveInput(XtInputId id) {
  for (int i = 0; i < wasm_input_count_; i++) {
    if (wasm_inputs_[i].id == id) {
      wasm_inputs_[i].removed = true;
      return;
    }
  }
}

void XtGetApplicationResources(Widget, void *base, XtResource *res, unsigned int n, void *, unsigned int) {
  for (unsigned int i = 0; i < n; i++) {
    const char *found_val = NULL;
    for (int j = 0; j < num_parsed_resources_; j++) {
      if (strcmp(res[i].resource_name, parsed_resources_[j].name) == 0) {
        found_val = parsed_resources_[j].value;
        break;
      }
    }
    
    if (found_val) {
      if (strcmp(res[i].resource_type, "Boolean") == 0) {
        Boolean val = (strcmp(found_val, "True") == 0 || strcmp(found_val, "1") == 0);
        memcpy((char *)base + res[i].resource_offset, &val, res[i].resource_size);
      } else if (strcmp(res[i].resource_type, "Integer") == 0 || strcmp(res[i].resource_type, "Int") == 0) {
        int val = atoi(found_val);
        memcpy((char *)base + res[i].resource_offset, &val, res[i].resource_size);
      } else if (strcmp(res[i].resource_type, "String") == 0) {
        const char *val = found_val;
        memcpy((char *)base + res[i].resource_offset, &val, res[i].resource_size);
      } else {
        memcpy((char *)base + res[i].resource_offset, &res[i].default_addr, res[i].resource_size < sizeof(void *) ? res[i].resource_size : sizeof(void *));
      }
    } else {
      if (res[i].resource_size < sizeof(void *)) {
        memcpy((char *)base + res[i].resource_offset, &res[i].default_addr, res[i].resource_size);
      } else {
        memcpy((char *)base + res[i].resource_offset, &res[i].default_addr, sizeof(void *));
      }
    }
  }
}

XtIntervalId XtAppAddTimeOut(XtAppContext, unsigned long ms, void (*cb)(), XtPointer data) {
  if (wasm_pending_timeouts_ > 512)
    ms = 100;
  BTWasmTimeout *t = (BTWasmTimeout *)malloc(sizeof(BTWasmTimeout));
  t->cb = (void (*)(XtPointer, XtIntervalId *))cb;
  t->data = data;
  t->cancelled = false;
  wasm_pending_timeouts_++;
  emscripten_async_call(timeout_tramp, t, (int)ms);
  return (XtIntervalId)t;
}

void XtRemoveTimeOut(XtIntervalId id) {
  if (id) {
    BTWasmTimeout *t = (BTWasmTimeout *)id;
    t->cancelled = true;
  }
}

Display *XtDisplay(Widget) { return &display_; }
Window XtWindow(Widget w) { return (Window)(unsigned long)w; }
Screen *XtScreen(Widget) { return &screen_; }

XmString XmStringCreateSimple(char *s) {
  return s ? (XmString)strdup(s) : NULL;
}
XmString XmStringCreateLtoR(char *s, char *) {
  return s ? (XmString)strdup(s) : NULL;
}
XmString XmStringConcat(XmString a, XmString b) {
  if (!a && !b) return NULL;
  if (!a) return (XmString)strdup((char *)b);
  if (!b) return (XmString)strdup((char *)a);
  size_t len_a = strlen((char *)a);
  size_t len_b = strlen((char *)b);
  char *res = (char *)malloc(len_a + len_b + 1);
  memcpy(res, a, len_a);
  memcpy(res + len_a, b, len_b);
  res[len_a + len_b] = '\0';
  return (XmString)res;
}
XmString XmStringSeparatorCreate(void) {
  return (XmString)strdup("\n");
}
void XmStringFree(void *s) {
  if (s) free(s);
}
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

Pixmap XCreatePixmap(Display *, Window, unsigned int w, unsigned int h, unsigned int) {
  static Pixmap p = 1000;
  Pixmap id = p++;
  EM_ASM({ if (Module.createDrawable) Module.createDrawable($0, $1, $2); }, id, w, h);
  return id;
}

Colormap XCreateColormap(Display *, Window, Visual *, int) { return 0; }
Colormap XCopyColormapAndFree(Display *, Colormap c) { return c; }

Window XCreateWindow(Display *, Window, int, int, unsigned int w, unsigned int h, unsigned int, int, unsigned int, Visual *, unsigned long, void *) {
  static Window w_id = 10000;
  Window id = w_id++;
  EM_ASM({ if (Module.createDrawable) Module.createDrawable($0, $1, $2); }, id, w, h);
  return id;
}

GC XCreateGC(Display *, Window, unsigned long, XGCValues *values) {
  BTWasmGC *gc = (BTWasmGC *)calloc(1, sizeof(BTWasmGC));
  if (values) {
    gc->foreground = values->foreground;
  }
  return (GC)gc;
}

void XFreeGC(Display *, GC gc) {
  free((void *)gc);
}

void XFreeColormap(Display *, Colormap) {}

void XFreePixmap(Display *, Pixmap p) {
  EM_ASM({ if (Module.destroyDrawable) Module.destroyDrawable($0); }, p);
}

void XFree(void *p) { free(p); }

void XDestroyWindow(Display *, Window w) {
  EM_ASM({ if (Module.destroyDrawable) Module.destroyDrawable($0); }, w);
}

void XFlush(Display *) {
  EM_ASM({ if (Module.btFlush) Module.btFlush(); });
}

void XSync(Display *, Boolean) {}
void XGetErrorText(Display *, int, char *buf, int len) { if (len > 0) strncpy(buf, "wasm x error", len); }
char *DisplayString(Display *) { return (char *)"wasm"; }

void XSetForeground(Display *, GC gc, unsigned long pixel) {
  if (gc) {
    ((BTWasmGC *)gc)->foreground = pixel;
  }
}

void XSetBackground(Display *, GC gc, unsigned long pixel) {
  if (gc) {
    ((BTWasmGC *)gc)->background = pixel;
  }
}

void XFillRectangle(Display *, unsigned long dest, GC gc, int x, int y, unsigned int w, unsigned int h) {
  unsigned long color = gc ? ((BTWasmGC *)gc)->foreground : 0;
  EM_ASM({
    if (Module.fillRectangle) Module.fillRectangle($0, $1, $2, $3, $4, $5);
  }, dest, color, x, y, w, h);
}

void XFillArc(Display *, unsigned long dest, GC gc, int x, int y, unsigned int w, unsigned int h, int angle1, int angle2) {
  unsigned long color = gc ? ((BTWasmGC *)gc)->foreground : 0;
  EM_ASM({
    if (Module.fillArc) Module.fillArc($0, $1, $2, $3, $4, $5, $6, $7);
  }, dest, color, x, y, w, h, angle1, angle2);
}

void XDrawArc(Display *, unsigned long dest, GC gc, int x, int y, unsigned int w, unsigned int h, int angle1, int angle2) {
  unsigned long color = gc ? ((BTWasmGC *)gc)->foreground : 0;
  EM_ASM({
    if (Module.drawArc) Module.drawArc($0, $1, $2, $3, $4, $5, $6, $7);
  }, dest, color, x, y, w, h, angle1, angle2);
}

void XDrawPoint(Display *, unsigned long dest, GC gc, int x, int y) {
  unsigned long color = gc ? ((BTWasmGC *)gc)->foreground : 0;
  EM_ASM({
    if (Module.fillRectangle) Module.fillRectangle($0, $1, $2, $3, 1, 1);
  }, dest, color, x, y);
}

void XCopyArea(Display *, unsigned long src, unsigned long dest, GC gc, int src_x, int src_y, unsigned int w, unsigned int h, int dest_x, int dest_y) {
  EM_ASM({
    if (Module.copyArea) Module.copyArea($0, $1, $2, $3, $4, $5, $6, $7);
  }, src, dest, src_x, src_y, w, h, dest_x, dest_y);
}

XImage *XCreateImage(Display *, Visual *, unsigned int, int, int, char *data, unsigned int w, unsigned int h, int, int) {
  XImage *img = (XImage *)calloc(1, sizeof(XImage));
  img->width = w;
  img->height = h;
  img->bytes_per_line = w * 4;
  if (data) {
    img->data = data;
  } else {
    img->data = (char *)malloc(img->bytes_per_line * h);
  }
  return img;
}

void XDestroyImage(XImage *img) {
  if (img) {
    free(img->data);
    free(img);
  }
}

int XPutImage(Display *, unsigned long dest, GC gc, XImage *image, int src_x, int src_y, int dest_x, int dest_y, unsigned int w, unsigned int h) {
  if (image && image->data) {
    EM_ASM({
      if (Module.putImage) Module.putImage($0, $1, $2, $3, $4, $5, $6, $7);
    }, dest, (unsigned long)image->data, image->width, image->height, src_x, src_y, dest_x, dest_y);
  }
  return 0;
}

int XBitmapPad(Display *) { return 32; }

int XAllocColor(Display *, Colormap, XColor *c) {
  if (c) {
    unsigned char r = c->red >> 8;
    unsigned char g = c->green >> 8;
    unsigned char b = c->blue >> 8;
    c->pixel = (r << 16) | (g << 8) | b;
  }
  return 1;
}

void XPutPixel(XImage *img, int x, int y, unsigned long pixel) {
  if (img && img->data) {
    unsigned int *pixels = (unsigned int *)img->data;
    pixels[y * img->width + x] = pixel;
  }
}

int XLookupString(XKeyEvent *, char *buf, int len, KeySym *keysym, XComposeStatus *) {
  if (len > 0) {
    buf[0] = last_key_char;
    buf[1] = 0;
    if (keysym) *keysym = last_key_char;
    return 1;
  }
  return 0;
}

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

void XmTextSetString(Widget w, char *s) {
  if (w) {
    if (w->label_string) free(w->label_string);
    w->label_string = s ? strdup(s) : NULL;
    update_widget_js(w);
  }
}

void XmTextReplace(Widget w, int start, int end, char *s) {
  if (w) {
    const char *old = w->label_string ? w->label_string : "";
    int old_len = strlen(old);
    int repl_len = s ? strlen(s) : 0;
    if (start < 0) start = 0;
    if (end < start) end = start;
    if (end > old_len) end = old_len;
    char *next = (char *)malloc(old_len - (end - start) + repl_len + 1);
    memcpy(next, old, start);
    if (repl_len) memcpy(next + start, s, repl_len);
    strcpy(next + start + repl_len, old + end);
    if (w->label_string) free(w->label_string);
    w->label_string = next;
    update_widget_js(w);
  }
}

void XmListSelectItem(Widget w, XmString item, Boolean notify) {
  if (w && item) {
    if (w->label_string) free(w->label_string);
    w->label_string = strdup((char *)item);
    update_widget_js(w);
    if (notify && w->activate) {
      XmListCallbackStruct cbs;
      cbs.reason = XmCR_ACTIVATE;
      cbs.item = item;
      cbs.item_length = strlen((char *)item);
      cbs.item_string = (char *)item;
      w->activate(w, w->activate_data, &cbs);
    }
  }
}

void XmListSelectPos(Widget w, int pos, Boolean) {
  if (w) {
    w->slider_value = pos;
    update_widget_js(w);
  }
}

void XmListDeselectAllItems(Widget) {}
void XmListDeleteAllItems(Widget) {}

extern "C" {
  EMSCRIPTEN_KEEPALIVE void bt_wasm_activate(Widget w) {
    if (w && w->activate) {
      XmAnyCallbackStruct cbs;
      cbs.reason = XmCR_ACTIVATE;
      w->activate(w, w->activate_data, &cbs);
    }
  }

  EMSCRIPTEN_KEEPALIVE void bt_wasm_scale_changed(Widget w, int val) {
    if (w) {
      w->slider_value = val;
      update_widget_js(w);
      if (w->activate) {
        XmScaleCallbackStruct cbs;
        cbs.reason = XmCR_VALUE_CHANGED;
        cbs.value = val;
        w->activate(w, w->activate_data, &cbs);
      }
    }
  }

  EMSCRIPTEN_KEEPALIVE void bt_wasm_toggle_changed(Widget w, int set) {
    if (w) {
      w->set = set;
      update_widget_js(w);
      if (w->activate) {
        XmToggleButtonCallbackStruct cbs;
        cbs.set = set;
        w->activate(w, w->activate_data, &cbs);
      }
    }
  }

  EMSCRIPTEN_KEEPALIVE void bt_wasm_list_selected(Widget w, const char *item, int pos) {
    if (w) {
      if (w->label_string) free(w->label_string);
      w->label_string = item ? strdup(item) : NULL;
      w->slider_value = pos;
      update_widget_js(w);
      if (w->activate) {
        XmListCallbackStruct cbs;
        cbs.reason = XmCR_ACTIVATE;
        cbs.item = (void *)item;
        cbs.item_length = item ? strlen(item) : 0;
        cbs.item_string = (char *)item;
        cbs.item_position = pos;
        w->activate(w, w->activate_data, &cbs);
      }
    }
  }

  EMSCRIPTEN_KEEPALIVE void bt_wasm_keypress(Widget w, char key) {
    if (w && w->kbd_handler) {
      last_key_char = key;
      XEvent ev;
      ev.type = KeyPressMask;
      ev.xkey.type = KeyPressMask;
      ev.xkey.keycode = (unsigned int)key;
      w->kbd_handler(w, w->kbd_data, &ev, 0);

      const char *action = 0;
      switch (key) {
      case 'j':
      case 'J':
        action = "move_left";
        break;
      case 'l':
      case 'L':
        action = "move_right";
        break;
      case 'k':
      case 'K':
        action = "rotate";
        break;
      case ' ':
        action = "drop";
        break;
      case 'p':
      case 'P':
        action = "pause";
        break;
      case 'c':
      case 'C':
        action = "condor";
        break;
      }

      if (action) {
        for (unsigned int i = 0; i < wasm_action_count_; i++) {
          if (!strcmp(wasm_actions_[i].name, action)) {
            unsigned int nparams = 0;
            wasm_actions_[i].proc(w, &ev, 0, &nparams);
            break;
          }
        }
      }
    }
  }

  EMSCRIPTEN_KEEPALIVE void bt_wasm_keypress_any(char key) {
    EM_ASM({ if (Module.btPhase) Module.btPhase("key"); });
    if (wasm_keyboard_widget_) bt_wasm_keypress(wasm_keyboard_widget_, key);
    EM_ASM({ if (Module.btPhase) Module.btPhase("idle"); });
  }

  EMSCRIPTEN_KEEPALIVE void bt_wasm_input_event(Widget w, int type, int x, int y) {
    if (w && w->input_cb) {
      XEvent ev;
      ev.type = type;
      XmDrawingAreaCallbackStruct cbs;
      cbs.reason = XmCR_EXPOSE;
      cbs.event = &ev;
      w->input_cb(w, w->input_data, &cbs);
    }
  }
}
