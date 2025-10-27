# Compilatore e flag
CXX       := g++
CXXFLAGS  := -std=c++11 -Wall -Wextra -Wpedantic -O0 -g -I.
LDFLAGS   :=
LDLIBS    :=

# Struttura cartelle
BIN_DIR   := bin
OBJ_DIR   := build/obj
DATASET_DIR := datasets
LIST_FILE := datasets.txt

# Sorgenti dell'eseguibile principale
SRCS := \
  main.cpp \
  src/utils/MatrixReader.cpp \
  src/serial/SpMVSerial.cpp \
  src/utils/PrintUtils.cpp \
  src/utils/ExecutionStatistics.cpp \
  src/interfaces/SpVMInterface.cpp

# Oggetti e dipendenze
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

# Sezione download/estrazione datasets
.PHONY: datasets clean-datasets list-datasets

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