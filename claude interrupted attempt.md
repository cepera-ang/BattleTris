Here is the complete rendering pipeline analysis for BattleTris:

---

## 1. BTGame.C — `_gamecb2_` Expose Callback

**Definition** in BTGame.H:159-161:

cpp

static void _gamecb2_(BTWidget *, void *d) {

    ((BTGame *)d)->exposeEvent();

}

It is registered at BTGame.C:103:

cpp

drawing_area_->addExposeCallback( _gamecb2_, this );

**`exposeEvent()`** is at BTGame.C:391-425. It does:

- **First call** (`!initialized_`): Creates `BTBoardManager` and `BTPieceManager`, wires up the token ring chain (`board_ -> piece_manager_ -> sound_manager_ -> score_manager_ -> comm_manager_ -> weapon_manager_ -> recon_ -> this`), then calls `board_->redraw()`.
- **First start** (`!started_`): Calls `reset()`, optionally `condor()`, `board_->redraw()`, sets `started_=1`, then `startGame()`.
- **Subsequent calls**: Redraws the current falling piece (`current_piece_->redraw()`) and the board (`board_->redraw()`).

The drawing area widget is created at BTGame.C:77-80 with dimensions `BT_BOX_WTH * BT_BOARD_WTH` × `BT_BOX_HGT * BT_BOARD_HGT`, positioned at `(BT_BOARD_X, BT_BOARD_Y)`.

---

## 2. BTBoard.C vs BTBoardManager.C — The `redraw()` Method

**BTBoard.C** (51 lines) is a _data-only_ class — it stores a serialized representation of the board as `rep_[]` (a flat array). It has **NO draw/redraw methods**. It is used for serialization (sending boards over the network).

**BTBoardManager.C** has the actual `redraw()` at BTBoardManager.C:619-625:

cpp

void BTBoardManager::redraw() {

  for (int i = 0; i < height_; i++)

    for (int j = 0; j < width_; j++)

      if (map_[j][i]) 

        map_[j][i]->redraw();   // Each cell is a BTBox*

  if (!computer_) DISPLAY->flush();

}

This iterates all cells in the `map_[width][height]` grid. Each occupied cell is a `BTBox*`, and it calls `BTBox::redraw()` on each. The board itself does **no direct X11 calls** — it delegates entirely to its BTBox objects.

---

## 3. BTBox.C — Individual Tetris Box Drawing

### BTBox::redraw() — BTBox.C:281-292

cpp

void BTBox::redraw() {

  if ( ! display_ ) return;

  if ( hidden_ ) { erase(); return; }

  if ( pixmap_ )

    XCopyArea( display_, *pixmap_, window_, black_gc,

               0, 0, BT_BOX_WTH, BT_BOX_HGT, x_, y_ );

}

**Key: Each box draws by `XCopyArea`-ing a pre-rendered pixmap onto `window_`.**

### BTBox::erase() — BTBox.C:294-299

cpp

void BTBox::erase() {

  if ( ! display_ ) return;

  XFillRectangle (display_, window_, black_gc, x_, y_, BT_BOX_WTH, BT_BOX_HGT);

}

### BTBox::moveTo() — BTBox.C:271-279

Erases old position, computes new pixel coords (`x_ = x * BT_BOX_WTH; y_ = y * BT_BOX_HGT`), then redraws.

### BTBox target drawable:

The `window_` field is set in BTBox.C:42:

cpp

window_ = XtWindow(da->getWidget());

where `da` is the BTDrawingAreaWidget passed to `BTBoxManager`. So **all boxes draw directly onto the drawing area widget's Window**.

### BTBoxManager pixmap creation — BTBox.C:35-172:

The `BTBoxManager` constructor pre-renders all box types into off-screen `BTPixmap` objects (`box_maps[]` global array):

- **Normal colored boxes** (lines 70-76): Two `XFillRectangle` calls — a darker color for the full box, then a lighter color for the main area minus a border (creating a 3D bevel effect).
- **Structure/neutral boxes** (lines 78-80): Single `XFillRectangle`.
- **Dice boxes** (lines 82-130): Ivory background with black dots arranged for each die face, using `XFillRectangle` calls.
- **Happy face boxes** (lines 132-146): Yellow background with `XFillArc` for eyes and `XDrawArc` for smile.
- **Unhappy face boxes** (lines 148-168): Same but with tear (`XDrawPoint`, `XFillArc`) and frown (`XDrawArc`).
- **Gimp boxes** (line 170-171): Use a custom `BTPixmap` passed in.

---

## 4. BTPieceManager.C — Piece Rendering

**BTPieceManager does NOT directly render pieces.** It manages piece _selection and lifecycle_:

- BTPieceManager.C:21-61: Constructor creates a `BTBoxManager` (line 26) and all piece types (lines 43-60). Each piece (e.g. `BTElPiece`, `BTLongPiece`, etc.) is a `BTPiece` subclass.
- BTPieceManager.C:179-217: `create(x, y)` picks a random piece based on weighted probabilities, calls `piece->reset()` and `piece->construct(x, y)`.
- BTPieceManager.C:219-222: `dispose(old)` calls `old->landed()` then `old->reset()`.

Actual piece _drawing_ happens via the piece's constituent `BTBox` objects — each `BTPiece` is composed of multiple `BTBox` instances, each of which calls `BTBox::redraw()` → `XCopyArea`.

---

## 5. BTRecon.C — Opponent Board Drawing

### Constructor — BTRecon.C:121-152:

Creates its own `BTDrawingAreaWidget` at position `(665, 30)` with same dimensions as main board (`BT_BOX_WTH * BT_BOARD_WTH` × `BT_BOX_HGT * BT_BOARD_HGT`). Registers `_reconcb2_` as expose callback.

### `_reconcb2_` — BTRecon.C:39-41:

cpp

void BTRecon::_reconcb2_ (BTWidget *w, void *data) {

  ((BTRecon *)data)->exposeEvent();

}

### `exposeEvent()` — BTRecon.C:43-52:

On first call, creates its own `BTBoxManager`. On subsequent calls, redraws all non-null entries in `map_[]`.

### `drawBoard()` — BTRecon.C:54-92:

Receives a `BTBoard` (serialized opponent board) and renders it into the recon drawing area using `box_manager_->createByID()` and `map_[offset]->redraw()`. Has probabilistic display based on spy type (Ames=50%, Ace=85%, Condor=100%).

### Visibility control:

- When a spy weapon is launched (BTRecon.C:176-182): resizes toplevel to `BT_RECON_BIG` (934) and `drawing_area_->manage()`.
- When condor off (BTRecon.C:188-199): resizes to `BT_RECON_SMALL` (670) and `drawing_area_->unmanage()`.

---

## 6. bt_wasm_xm.C — WASM Bridge

### `update_widget_js()` — bt_wasm_xm.C:147-195:

Static function that passes widget state to JavaScript via three `EM_ASM` calls to `Module.updateWidget()`:

1. **Lines 150-161**: Basic geometry & state: `x, y, width, height, managed, mapped, label, set, slider_value`
2. **Lines 164-176**: Attachments & constraints: `minimum, maximum, fraction_base`, all attachment types and left/right positions
3. **Lines 179-194**: Offsets & target widgets: `top_position, bottom_position`, all offsets, all widget references

Called after every property change (`XtVaSetValues`, `XtSetValues`, `XtManageChild`, etc.).

### `fire_expose()` — bt_wasm_xm.C:269-277:

cpp

static void fire_expose(void *arg) {

  Widget w = (Widget)arg;

  if (w && w->expose) {

    XmDrawingAreaCallbackStruct cbs;

    cbs.reason = XmCR_EXPOSE;

    cbs.event = 0;

    w->expose(w, w->expose_data, &cbs);

  }

}

Creates a synthetic expose callback struct and invokes the widget's registered expose callback.

### Expose triggers:

- **`XtManageChild()`** line 453-461: If widget has expose callback, fires it asynchronously via `emscripten_async_call(fire_expose, w, 0)`.
- **`XtRealizeWidget()`** line 482-491: Also fires expose asynchronously, plus creates a drawable.
- **`XtAddCallback()` with `XmNexposeCallback`** line 499-503: Immediately fires expose async when callback is registered.

### X11 drawing calls forwarded to JS:

- `XFillRectangle` → `Module.fillRectangle(dest, color, x, y, w, h)` line 757-762
- `XCopyArea` → `Module.copyArea(src, dest, src_x, src_y, w, h, dest_x, dest_y)` line 785-789
- `XFillArc` → `Module.fillArc(dest, color, x, y, w, h, angle1, angle2)` line 764-769
- `XDrawArc` → `Module.drawArc(...)` line 771-776
- `XDrawPoint` → `Module.fillRectangle(dest, color, x, y, 1, 1)` line 778-783
- `XFlush` → `Module.btFlush()` line 737-739
- `XCreatePixmap` → `Module.createDrawable(id, w, h)` line 696-701
- `XtWindow()` simply casts the Widget pointer to a Window ID line 655

---

## Summary: Full Rendering Flow

Expose event (from XtManageChild/XtRealizeWidget)

  → fire_expose() → widget->expose callback

  → _gamecb2_ → BTGame::exposeEvent()

    → BTBoardManager::redraw()

      → for each cell: BTBox::redraw()

        → XCopyArea(display_, *pixmap_, window_, ...) 

          where window_ = drawing area widget (cast to Window ID)

          and pixmap_ = pre-rendered box_maps[color]

        → In WASM: Module.copyArea(src_pixmap_id, dest_widget_id, ...)

    → BTPiece::redraw() (current falling piece, same mechanism via BTBox)

    → DISPLAY->flush() → XFlush() → Module.btFlush()

All drawing goes to the drawing area widget's Window. Pre-rendered box pixmaps are XCopyArea'd onto it. In WASM, Window IDs are just the widget pointer values cast to unsigned long.


> ## WASM Shim Layer Research Results
> 
> ### 1. `bt_wasm_xm.C` — Full Analysis (985 lines total)
> 
> ---
> 
> #### BTWasmWidget struct (Lines 9–60)
> 
> cpp
> 
> struct BTWasmWidget {
> 
>   const char *name;
> 
>   WidgetClass widget_class;
> 
>   Widget parent;
> 
>   int x, y, width, height;
> 
>   // Attachments (left/right/top/bottom: attachment, position, widget, offset)
> 
>   int left_attachment, right_attachment, top_attachment, bottom_attachment;
> 
>   int left_position, right_position, top_position, bottom_position;
> 
>   Widget left_widget, right_widget, top_widget, bottom_widget;
> 
>   int left_offset, right_offset, top_offset, bottom_offset;
> 
>   int fraction_base;           // Form resources
> 
>   char *label_string;          // Widget content
> 
>   int set;                     // toggle buttons
> 
>   int slider_value, minimum, maximum;  // scale widgets
> 
>   int item_count;              // list widgets
> 
>   // Callbacks
> 
>   void (*activate)(Widget, XtPointer, XtPointer);
> 
>   void *activate_data;
> 
>   void (*expose)(Widget, XtPointer, XtPointer);
> 
>   void *expose_data;
> 
>   void (*input_cb)(Widget, XtPointer, XtPointer);
> 
>   void *input_data;
> 
>   void (*kbd_handler)(Widget, XtPointer, XEvent *, Boolean *);
> 
>   void *kbd_data;
> 
>   bool managed;
> 
>   bool mapped;
> 
> };
> 
> ---
> 
> #### `update_widget_js` function (Lines 147–195)
> 
> This is called after every property change. It sends widget state to JS via 3 separate `EM_ASM` calls (because of EM_ASM's ~10-arg limit):
> 
> 1. **Call 1 (L150–161)**: Sends `x, y, width, height, managed, mapped, label, set, slider_value` via `Module.updateWidget(widgetPtr, {...})`
> 2. **Call 2 (L164–176)**: Sends `minimum, maximum, fraction_base, left/right/top/bottom_attachment, left/right_position`
> 3. **Call 3 (L179–194)**: Sends `top/bottom_position, left/right/top/bottom_offset, left/right/top/bottom_widget` (as unsigned long pointers)
> 
> ---
> 
> #### `fire_expose` function (Lines 269–277)
> 
> cpp
> 
> static void fire_expose(void *arg) {
> 
>   Widget w = (Widget)arg;
> 
>   if (w && w->expose) {
> 
>     XmDrawingAreaCallbackStruct cbs;
> 
>     cbs.reason = XmCR_EXPOSE;
> 
>     cbs.event = 0;
> 
>     w->expose(w, w->expose_data, &cbs);
> 
>   }
> 
> }
> 
> Called via `emscripten_async_call(fire_expose, w, 0)` from:
> 
> - `XtManageChild` (L458) — when widget has expose callback
> - `XtRealizeWidget` (L488) — when widget has expose callback
> - `XtAddCallback` for `XmNexposeCallback` (L502) — immediately on registration
> 
> ---
> 
> #### XtVaSetValues handling of XmNx/y/width/height (Lines 526–546 + 197–267)
> 
> `XtVaSetValues` (L526) iterates va_args, calls `parse_args()` for each attr/value pair, then calls `update_widget_js(w)`.
> 
> In `parse_args()` (L197–267), the geometry properties are simple direct assignments:
> 
> - L199: `XmNwidth` → `w->width = (int)val`
> - L200: `XmNheight` → `w->height = (int)val`
> - L201: `XmNx` → `w->x = (int)val`
> - L202: `XmNy` → `w->y = (int)val`
> 
> After parse_args, `update_widget_js(w)` sends the new geometry to JS.
> 
> ---
> 
> #### Flush mechanism — `XFlush` triggers `btFlush` (Line 737–739)
> 
> cpp
> 
> void XFlush(Display *) {
> 
>   EM_ASM({ if (Module.btFlush) Module.btFlush(); });
> 
> }
> 
> This is the **critical flush path**. The game calls flush via two routes:
> 
> 1. **`DISPLAY->flush()`** — `BTXDisplay::flush()` (BTXDisplay.H:29) calls `XFlush(display_)`, which triggers `Module.btFlush()`. Used extensively in game code: BTBoardManager.C:321,624; BTChallenge.C:194,242,250; BTGame.C:322,346,681,692,700,762,827; BTRecon.C:91
> 2. **Direct `XFlush()` calls** — BattleTris.C:592,603; BTDrawingAreaWidget.C:112
> 
> Both end up at the same shim which calls `Module.btFlush()` in JavaScript.
> 
> ---
> 
> #### `XtWindow()` mapping to drawable IDs (Line 655)
> 
> cpp
> 
> Window XtWindow(Widget w) { return (Window)(unsigned long)w; }
> 
> **The widget pointer IS the drawable ID.** The pointer address of the BTWasmWidget struct is cast directly to a Window (unsigned long). This same value is passed to `Module.createDrawable($0, ...)` in `XtRealizeWidget` (L486):
> 
> cpp
> 
> EM_ASM({ if (Module.createDrawable) Module.createDrawable($0, $1, $2); }, (Window)w, w->width, w->height);
> 
> For Pixmaps (`XCreatePixmap`, L696–701), drawable IDs are monotonically incrementing from 1000. For XCreateWindow (L706–711), IDs start from 10000.
> 
> ---
> 
> #### Exported C functions (Lines 911–984, `extern "C"` with `EMSCRIPTEN_KEEPALIVE`)
> 
> These are callable from JS:
> 
> - `bt_wasm_activate(Widget w)` — L912: fires widget's activate callback
> - `bt_wasm_scale_changed(Widget w, int val)` — L920: sets slider_value, fires value-changed
> - `bt_wasm_toggle_changed(Widget w, int set)` — L933: sets toggle state, fires callback
> - `bt_wasm_list_selected(Widget w, const char *item, int pos)` — L945: list selection
> - `bt_wasm_keypress(Widget w, char key)` — L963: dispatches keyboard event
> - `bt_wasm_input_event(Widget w, int type, int x, int y)` — L974: dispatches input event
> 
> ---
> 
> ### 2. `wasm/Makefile` (33 lines)
> 
> Full file at `c:\Users\cepera\projects\BattleTris-full-wasm\wasm\Makefile`.
> 
> **Key build flags:**
> 
> - **CXXFLAGS**: `-std=gnu++98 -D__EMSCRIPTEN__ -Dmain=bt_original_main` (renames main), plus `-DMKIFLAGS='"" ' -DMKCXXFLAGS='"wasm"'`, includes: `include/, game/, widget/, stdlib/, db/, sockets/, audio/, signals/`
> - **LDFLAGS**: `-s WASM=1 -s ASYNCIFY=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","HEAPU32"]'`, preloads `share` and `art` directories
> - **Compiler**: `/mnt/c/Users/cepera/projects/emsdk/upstream/emscripten/em++`
> - **Sources**: `bt_wasm_xm.C`, `bt_wasm_entry.C`, plus all game/widget/stdlib/db/sockets/audio/signals .C files
> - **Output**: `dist/battletris-full.js`
> 
> **`bt_wasm_entry.C` (9 lines)**: Simply `#undef main` then calls `bt_original_main(argc, argv)` — undoes the `-Dmain=bt_original_main` macro to provide the real entry point.
> 
> **Note**: No explicit `-s EXPORTED_FUNCTIONS` list — relies on `EMSCRIPTEN_KEEPALIVE` annotations in the C code for the `bt_wasm_*` functions.
> 
> ---
> 
> ### 3. BTDisplay / Flush Architecture
> 
> **`BTDisplay.H`** (usr/src/widget/BTDisplay.H) — Abstract base class (L15–30):
> 
> - `virtual void flush() {}` — default no-op (L24)
> - `virtual void newPalette() = 0`
> - `virtual void handleEvents() = 0`
> - `virtual unsigned long addTimeout(...)` = 0
> - `extern BTDisplay *DISPLAY;` — global singleton (L32)
> 
> **`BTXDisplay.H`** (usr/src/widget/BTXDisplay.H) — Concrete X11/WASM implementation (L17–34):
> 
> - `void flush() { XFlush(display_); }` — (L29) calls XFlush, which in the WASM shim calls `Module.btFlush()`
> - Has `Widget toplevel_`, `Display *display_`, `XtAppContext app_`
> 
> **`BTDisplay.C`** — Just includes BTDisplay.H, defines no code (the global `DISPLAY` pointer is likely set elsewhere).
> 
> ### Summary of the Complete Flush Pipeline:
> 
> Game code: DISPLAY->flush()
> 
>   → BTXDisplay::flush()  [BTXDisplay.H:29]
> 
>     → XFlush(display_)
> 
>       → WASM shim XFlush()  [bt_wasm_xm.C:737-739]
> 
>         → EM_ASM: Module.btFlush()
> 
>           → JavaScript renders all queued drawing ops

Research complete. All findings with exact line numbers have been sent to the main agent covering:

- BTWasmWidget struct definition (L9–60)
- `update_widget_js` full implementation (L147–195)
- `fire_expose` function (L269–277)
- `parse_args` handling of XmNx/y/width/height (L197–267)
- XFlush → Module.btFlush flush pipeline (L737–739)
- XtWindow → widget pointer cast (L655)
- Full Makefile analysis
- BTDisplay/BTXDisplay class hierarchy and flush architecture
  
  
  ### main agent work: 
- 
Now I have a clear picture of the issues. Let me now look at what happens in the `XFillRectangle` -> `fillRectangle` flow and the `XDisplayFlush` flow:

Good. Now I understand the full picture. The game draws to `XtWindow(drawing_area_)` which is the widget pointer cast to `unsigned long`. The JS `fillRectangle`, `copyArea` etc. use this as the drawable key. The `btFlush()` composites drawables onto the main canvas.

The key problems are:

1. **Keyboard input never reaches C++** — `update_widget_js` doesn't propagate `hasKbdHandler` to JS, so the JS keyboard handler at line 697 of `index.html` never calls `bt_wasm_keypress`
2. **The drawable for the game board may not be correctly sized** — when `size(30, 30)` is called, it sets `x=30, y=30` on the widget, but the drawable was initialized with `width=100, height=40` (default). The actual drawing area widget gets size via `BTDrawingAreaWidget` constructor which sets `width = BT_BOX_WTH * BT_BOARD_WTH` and `height = BT_BOX_HGT * BT_BOARD_HGT`.

Let me check what the actual drawing area dimensions should be:

So the drawing area is `BT_BOX_WTH * BT_BOARD_WTH = 23 * 10 = 230` wide and `BT_BOX_HGT * BT_BOARD_HGT` tall. Let me check `BT_BOX_HGT`:

So `230 x 644` (23*28=644). The widget dump shows `computedWidth=230, computedHeight=644` — that matches! The drawing area IS properly sized in the JS layout. The drawable is being created by `getOrCreateDrawable` with correct dimensions too since `createDrawable` is called in `XtRealizeWidget`.

The real question is: **is the C++ code actually drawing to the drawable?** The C++ draws to `XtWindow(drawing_area_)` which is the widget pointer as `unsigned long`. The JS `fillRectangle` gets this as `destId` and looks up `drawables[destId]`. But in `createWidget`, the widget is registered with `id = (unsigned long)w`, and the drawable is created in `XtRealizeWidget` with the same id. So this should work.

Let me look at the `BTDrawingAreaWidget` constructor to see where `XtRealizeWidget` is called:

Message from WASM shim researcher (research)

Excellent! The WASM shim researcher is back. Now I have the complete picture. `XtRealizeWidget` is only called once for `g_toplevel` (the shell). Individual drawing area widgets don't get realized — they get their drawables created lazily by `getOrCreateDrawable` in JS. But the key problem is the **kbd_handler not being propagated to JS**.

Let me wait for the other subagent, and then I'll create the implementation plan.

Message from BTGame rendering researcher (research)

Both research subagents are back. Now I have the complete picture. Let me analyze the issues and fix them. Here's what I've identified:

**Root Causes:**

1. **Keyboard input broken** — `update_widget_js` never sets `hasKbdHandler` on the JS widget, so JS keyboard handler never dispatches to C++
2. **Drawing area drawable not auto-created** — `XtRealizeWidget` is only called for the toplevel shell. The game's drawing area gets its drawable lazily via `getOrCreateDrawable`, but this uses default 100x40 size. The drawable needs to match the actual widget dimensions.
3. **Expose callback fires but draws to drawable that may be too small** — The C++ draws to `XtWindow(drawing_area_)` which creates a 100x40 drawable initially