ROOT := ..
SRC := $(ROOT)/usr/src
OUT := dist

CXXFLAGS := -std=gnu++98 -D__EMSCRIPTEN__ -Dmain=bt_original_main -DMKIFLAGS='""' -DMKLDFLAGS='""' -DMKCXXFLAGS='"wasm"' -DMKLIBS='""' -Iinclude -I$(SRC) -I$(SRC)/game -I$(SRC)/widget -I$(SRC)/stdlib -I$(SRC)/db -I$(SRC)/sockets -I$(SRC)/audio -I$(SRC)/signals -w
LDFLAGS := -s WASM=1 -s ASYNCIFY=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' --preload-file $(SRC)/share@/usr/src/share --preload-file $(SRC)/art@/usr/src/art

GAME := $(SRC)/game/BTCBoard.C $(SRC)/game/BTStack.C $(SRC)/game/BTBox.C $(SRC)/game/BTRingNode.C $(SRC)/game/BTPiece.C $(SRC)/game/BTPieceManager.C $(SRC)/game/BTArsenal.C $(SRC)/game/BTBoard.C $(SRC)/game/BTScore.C $(SRC)/game/BTBoardManager.C $(SRC)/game/BTSoundManager.C $(SRC)/game/BTScoreManager.C $(SRC)/game/BTBiff.C $(SRC)/game/BTStartup.C $(SRC)/game/BTCommManager.C $(SRC)/game/BTWeaponManager.C $(SRC)/game/BTRecon.C $(SRC)/game/BTGame.C $(SRC)/game/BTChallenge.C $(SRC)/game/BTChallengeDialog.C $(SRC)/game/BTAbout.C $(SRC)/game/BTNetManager.C $(SRC)/game/BTRoster.C $(SRC)/game/BTBazaar.C $(SRC)/game/BTPimp.C $(SRC)/game/BTComputer.C $(SRC)/game/PPMReader.C $(SRC)/game/BattleTris.C $(SRC)/game/BTFallbacks.C
WIDGET := $(SRC)/widget/BTWidget.C $(SRC)/widget/BTDrawingAreaWidget.C $(SRC)/widget/BTFormWidget.C $(SRC)/widget/BTLabelWidget.C $(SRC)/widget/BTMessageDlog.C $(SRC)/widget/BTPushButtonWidget.C $(SRC)/widget/BTRowColumnWidget.C $(SRC)/widget/BTSliderWidget.C $(SRC)/widget/BTScrolledListWidget.C $(SRC)/widget/BTScrolledTextWidget.C $(SRC)/widget/BTTextWidget.C $(SRC)/widget/BTPixmap.C $(SRC)/widget/BTFrameWidget.C $(SRC)/widget/BTDisplay.C $(SRC)/widget/BTXDisplay.C $(SRC)/widget/BTXPalette.C $(SRC)/widget/BTXmUtils.C
STD := $(SRC)/stdlib/AbsList.C $(SRC)/stdlib/AbsListElement.C $(SRC)/stdlib/AbsListIter.C $(SRC)/stdlib/List.C $(SRC)/stdlib/ListElement.C $(SRC)/stdlib/ListIter.C $(SRC)/stdlib/Block.C
DB := $(SRC)/db/BTDB.C $(SRC)/db/BTDBLock.C $(SRC)/db/BTDBRecord.C $(SRC)/db/BTDBErr.C $(SRC)/db/BTNetworkEntry.C $(SRC)/db/BTPlayer.C $(SRC)/db/BTPlayerRecord.C $(SRC)/db/BTGameStats.C $(SRC)/db/BTConfigFile.C $(SRC)/db/ParsedFile.C
SOCKETS := $(SRC)/sockets/Address.C $(SRC)/sockets/Socket.C $(SRC)/sockets/PacketBuffer.C $(SRC)/sockets/XtSocketCB.C $(SRC)/sockets/StreamSocketErr.C $(SRC)/sockets/StreamSocket.C
OTHER := $(SRC)/audio/DevAudio.C $(SRC)/signals/SigReceiver.C
SOURCES := bt_wasm_xm.C bt_wasm_entry.C $(GAME) $(WIDGET) $(STD) $(DB) $(SOCKETS) $(OTHER)
OBJ := $(addprefix obj/,$(notdir $(SOURCES:.C=.o)))
VPATH := . $(SRC)/game $(SRC)/widget $(SRC)/stdlib $(SRC)/db $(SRC)/sockets $(SRC)/audio $(SRC)/signals

all: $(OUT)/battletris-full.js

$(OUT)/battletris-full.js: $(OBJ) index.html style.css Makefile
	mkdir -p $(OUT)
	em++ $(OBJ) $(LDFLAGS) -o $@
	cp index.html style.css $(OUT)/

obj/%.o: %.C
	mkdir -p obj
	em++ $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OUT) obj
