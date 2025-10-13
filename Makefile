# Makefile

# Compilatore e flag
CXX       := g++
CXXFLAGS  := -std=c++20 -Wall -Wextra -Wpedantic -O0 -g -I.
LDFLAGS   :=
LDLIBS    :=

# Se usi GCC < 9 (per std::filesystem), scommenta la riga seguente:
# LDLIBS   += -lstdc++fs

# Struttura cartelle
BIN_DIR   := bin
OBJ_DIR   := build/obj

# Sorgenti dell'eseguibile principale
SRCS := \
  main.cpp \
  src/utils/MatrixReader.cpp \
  src/serial/SpMVSerial.cpp \
  src/utils/PrintUtils.cpp \
  src/utils/ExecutionStatistics.cpp \
  src/interfaces/SpVMInterface.cpp

# Oggetti e dipendenze (correzione: .cpp -> .o)
OBJS := $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# Target finale
TARGET := $(BIN_DIR)/deliverable1_2025_2026

# Target di default
.PHONY: all
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

# Compile con generazione automatica dipendenze
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Esegui
.PHONY: run
run: $(TARGET)
	$(TARGET)

# Pulisci
.PHONY: clean
clean:
	$(RM) -r $(BIN_DIR) $(OBJ_DIR)

# Includi dipendenze generate
-include $(DEPS)
