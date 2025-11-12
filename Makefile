# Compilatore
CXX := g++-9.1.0


# Flag di base
BASE_CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O0 -g -I.

# Flag specifici per cartella (personalizzabili)
CXXFLAGS_MAIN    := $(BASE_CXXFLAGS)
CXXFLAGS_GENERIC := $(BASE_CXXFLAGS)
CXXFLAGS_SERIAL  := $(BASE_CXXFLAGS)
CXXFLAGS_OMP     := $(BASE_CXXFLAGS) -fopenmp

# Flag per il linker
LDFLAGS := -fopenmp
LDLIBS  :=

# Struttura cartelle
BIN_DIR     := bin
OBJ_DIR     := build/obj
DATASET_DIR := datasets
LIST_FILE   := datasets.txt

# Sorgenti raggruppati per tipo
SRCS_MAIN    := main.cpp
SRCS_GENERIC := $(wildcard src/utils/*.cpp src/interfaces/*.cpp)
SRCS_SERIAL  := $(wildcard src/serial/*.cpp)
# Sorgenti specifici per compilare solo la versione seriale
SRCS_SERIAL_ONLY := src/serial/main_serial.cpp src/serial/SpMVSerial.cpp
# Sorgenti specifici per compilare solo la versione OpenMP
SRCS_OMP_ONLY := src/openMP/main_openmp_static.cpp src/openMP/SpMVOpenMPStatic.cpp
SRCS_OMP_BINNING_ONLY := src/openMP/main_openmp_binning.cpp src/openMP/SpMVOpenMPBinning.cpp
SRCS_OMP_DYNAMIC_ONLY := src/openMP/main_openmp_dynamic.cpp src/openMP/SpMVOpenMPDynamic.cpp
# CORREZIONE: Corretto il percorso da 'openmp' a 'openMP'
SRCS_OMP     := $(wildcard src/openMP/*.cpp)

# Oggetti e dipendenze
OBJS_MAIN    := $(SRCS_MAIN:%.cpp=$(OBJ_DIR)/%.o)
OBJS_GENERIC := $(SRCS_GENERIC:%.cpp=$(OBJ_DIR)/%.o)
OBJS_SERIAL  := $(SRCS_SERIAL:%.cpp=$(OBJ_DIR)/%.o)
OBJS_SERIAL_ONLY := $(SRCS_SERIAL_ONLY:%.cpp=$(OBJ_DIR)/%.o)
OBJS_OMP_ONLY := $(SRCS_OMP_ONLY:%.cpp=$(OBJ_DIR)/%.o)
OBJS_OMP_BINNING_ONLY := $(SRCS_OMP_BINNING_ONLY:%.cpp=$(OBJ_DIR)/%.o)
OBJS_OMP_DYNAMIC_ONLY := $(SRCS_OMP_DYNAMIC_ONLY:%.cpp=$(OBJ_DIR)/%.o)
OBJS_OMP     := $(SRCS_OMP:%.cpp=$(OBJ_DIR)/%.o)
OBJS         := $(OBJS_MAIN) $(OBJS_GENERIC) $(OBJS_SERIAL) $(OBJS_OMP)
DEPS         := $(OBJS:.o=.d)

# Target finale
TARGET := $(BIN_DIR)/deliverable1_2025_2026
# Target specifico seriale
TARGET_SERIAL := $(BIN_DIR)/serial_spmv
# Target specifico OpenMP
TARGET_OMP := $(BIN_DIR)/openmp_spmv

TARGET_OMP_BINNING := $(BIN_DIR)/openmp_spmv_binning

TARGET_OMP_DYNAMIC := $(BIN_DIR)/openmp_spmv_dynamic


.PHONY: all run clean datasets list-datasets clean-datasets serial-only openmp-static-only openmp-binning-only openmp-dynamic-only

all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

# Regola per compilare solo main_serial e SpMVSerial (più oggetti generici)
serial-only: $(TARGET_SERIAL)

$(TARGET_SERIAL): $(OBJS_SERIAL_ONLY) $(OBJS_GENERIC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS_SERIAL_ONLY) $(OBJS_GENERIC) -o $@ $(LDFLAGS) $(LDLIBS)

# Regola per compilare solo main_openmp e SpVMOpenMP (più oggetti generici)
openmp-static-only: $(TARGET_OMP)

$(TARGET_OMP): $(OBJS_OMP_ONLY) $(OBJS_GENERIC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS_OMP_ONLY) $(OBJS_GENERIC) -o $@ $(LDFLAGS) $(LDLIBS)

openmp-binning-only: $(TARGET_OMP_BINNING)

$(TARGET_OMP_BINNING): $(OBJS_OMP_BINNING_ONLY) $(OBJS_GENERIC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS_OMP_BINNING_ONLY) $(OBJS_GENERIC) -o $@ $(LDFLAGS) $(LDLIBS)

openmp-dynamic-only: $(TARGET_OMP_DYNAMIC)

$(TARGET_OMP_DYNAMIC): $(OBJS_OMP_DYNAMIC_ONLY) $(OBJS_GENERIC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS_OMP_DYNAMIC_ONLY) $(OBJS_GENERIC) -o $@ $(LDFLAGS) $(LDLIBS)

# Regole di compilazione specifiche
$(OBJ_DIR)/main.o: main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_MAIN) -MMD -MP -c $< -o $@

$(OBJ_DIR)/src/utils/%.o: src/utils/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_GENERIC) -MMD -MP -c $< -o $@

$(OBJ_DIR)/src/interfaces/%.o: src/interfaces/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_GENERIC) -MMD -MP -c $< -o $@

$(OBJ_DIR)/src/serial/%.o: src/serial/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_SERIAL) -MMD -MP -c $< -o $@

# CORREZIONE: Corretto il percorso da 'openmp' a 'openMP'
$(OBJ_DIR)/src/openMP/%.o: src/openMP/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_OMP) -MMD -MP -c $< -o $@

# Esegui
run: $(TARGET)
	./$(TARGET)

# Pulisci
clean:
	$(RM) -r $(BIN_DIR) $(OBJ_DIR)

# Sezione download/estrazione datasets
datasets: $(LIST_FILE)
	@mkdir -p $(DATASET_DIR)
	@set -e; \
	while IFS= read -r url || [ -n "$$url" ]; do \
	 [ -z "$$url" ] && continue; \
	 case "$$url" in \#*) continue;; esac; \
	 echo "Processando: $$url"; \
	 fname=$${url##*/}; \
	 archive_path="$(DATASET_DIR)/$$fname"; \
	 if command -v wget >/dev/null 2>&1; then \
	  echo "Scarico $$url"; \
	  wget -q -c -P "$(DATASET_DIR)" "$$url"; \
	 elif command -v curl >/dev/null 2>&1; then \
	  echo "Scarico $$url"; \
	  curl -L --fail --retry 3 -o "$$archive_path" "$$url"; \
	 else \
	  echo "Errore: servono wget o curl" >&2; exit 1; \
	 fi; \
	 case "$$fname" in \
	  *.tar.gz|*.tgz) \
	   tmpdir="$(DATASET_DIR)/.tmp_extract_$$PPID.$$RANDOM"; \
	   mkdir -p "$$tmpdir"; \
	   echo "Estraggo $$fname in $$tmpdir"; \
	   tar -xzf "$$archive_path" -C "$$tmpdir"; \
	   mtx_count=$$(find "$$tmpdir" -type f -name '*.mtx' | wc -l | tr -d '[:space:]'); \
	   if [ "$$mtx_count" -eq 0 ]; then \
	    echo "Attenzione: nessun .mtx trovato in $$fname"; \
	   else \
	    echo "Trovati $$mtx_count file .mtx, spostamento in $(DATASET_DIR)"; \
	    find "$$tmpdir" -type f -name '*.mtx' -exec mv -t "$(DATASET_DIR)" {} +; \
	   fi; \
	   rm -rf "$$tmpdir"; \
	   rm -f "$$archive_path"; \
	   ;; \
	  *.tar) \
	   tmpdir="$(DATASET_DIR)/.tmp_extract_$$PPID.$$RANDOM"; \
	   mkdir -p "$$tmpdir"; \
	   echo "Estraggo $$fname in $$tmpdir"; \
	   tar -xf "$$archive_path" -C "$$tmpdir"; \
	   mtx_count=$$(find "$$tmpdir" -type f -name '*.mtx' | wc -l | tr -d '[:space:]'); \
	   if [ "$$mtx_count" -eq 0 ]; then \
	    echo "Attenzione: nessun .mtx trovato in $$fname"; \
	   else \
	    echo "Trovati $$mtx_count file .mtx, spostamento in $(DATASET_DIR)"; \
	    find "$$tmpdir" -type f -name '*.mtx' -exec mv -t "$(DATASET_DIR)" {} +; \
	   fi; \
	   rm -rf "$$tmpdir"; \
	   rm -f "$$archive_path"; \
	   ;; \
	  *.mtx) \
	   echo "File .mtx: nessuna estrazione necessaria"; \
	   ;; \
	  *) \
	   echo "Formato non supportato: $$fname"; \
	   rm -f "$$archive_path"; \
	   ;; \
	 esac; \
	done < "$(LIST_FILE)"
	@echo "Operazione completata. Solo i file .mtx sono disponibili in $(DATASET_DIR)"

list-datasets: $(LIST_FILE)
	@awk '!/^[[:space:]]*(#|$$)/ {print}' "$(LIST_FILE)"

clean-datasets:
	@rm -rf "$(DATASET_DIR)"

# Includi dipendenze generate
-include $(DEPS)
