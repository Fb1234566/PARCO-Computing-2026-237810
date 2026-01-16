# Compiler
CXX := g++
MPICC := mpicc


# Base flags
BASE_CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -g -I.

# Folder-specific flags (customizable)
CXXFLAGS_MAIN    := $(BASE_CXXFLAGS)
CXXFLAGS_GENERIC := $(BASE_CXXFLAGS)
CXXFLAGS_SERIAL  := $(BASE_CXXFLAGS)
CXXFLAGS_OMP     := $(BASE_CXXFLAGS) -fopenmp
CXXFLAGS_MPI     :=

# Linker flags
LDFLAGS := -fopenmp
LDLIBS  :=

# Directory structure
BIN_DIR     := bin
OBJ_DIR     := build/obj
DATASET_DIR := datasets
LIST_FILE   := datasets.txt

# Sources grouped by type
SRCS_MAIN    := main.cpp
SRCS_GENERIC := $(wildcard src/utils/*.cpp src/interfaces/*.cpp)
SRCS_SERIAL  := $(wildcard src/serial/*.cpp)
# Sources specifically to build only the serial version
SRCS_SERIAL_ONLY := src/serial/main_serial.cpp src/serial/SpMVSerial.cpp
# Sources specifically to build only the OpenMP version
SRCS_OMP_ONLY := src/openMP/main_openmp_static.cpp src/openMP/SpMVOpenMPStatic.cpp
SRCS_OMP_BINNING_ONLY := src/openMP/main_openmp_binning.cpp src/openMP/SpMVOpenMPBinning.cpp
SRCS_OMP_DYNAMIC_ONLY := src/openMP/main_openmp_dynamic.cpp src/openMP/SpMVOpenMPDynamic.cpp
SRCS_OMP_GUIDED_ONLY := src/openMP/main_openmp_guided.cpp src/openMP/SpMVOpenMPGuided.cpp
SRCS_OMP     := $(wildcard src/openMP/*.cpp)

SRCS_MPI := $(wildcard src/mpi/*.c)

# Objects and dependencies
OBJS_MAIN    := $(SRCS_MAIN:%.cpp=$(OBJ_DIR)/%.o)
OBJS_GENERIC := $(SRCS_GENERIC:%.cpp=$(OBJ_DIR)/%.o)
OBJS_SERIAL  := $(SRCS_SERIAL:%.cpp=$(OBJ_DIR)/%.o)
OBJS_SERIAL_ONLY := $(SRCS_SERIAL_ONLY:%.cpp=$(OBJ_DIR)/%.o)
OBJS_OMP_ONLY := $(SRCS_OMP_ONLY:%.cpp=$(OBJ_DIR)/%.o)
OBJS_OMP_BINNING_ONLY := $(SRCS_OMP_BINNING_ONLY:%.cpp=$(OBJ_DIR)/%.o)
OBJS_OMP_GUIDED_ONLY := $(SRCS_OMP_GUIDED_ONLY:%.cpp=$(OBJ_DIR)/%.o)
OBJS_OMP_DYNAMIC_ONLY := $(SRCS_OMP_DYNAMIC_ONLY:%.cpp=$(OBJ_DIR)/%.o)
OBJS_OMP     := $(SRCS_OMP:%.cpp=$(OBJ_DIR)/%.o)
OBJS_MPI := $(SRCS_MPI:%.c=$(OBJ_DIR)/%.o)
OBJS         := $(OBJS_MAIN) $(OBJS_GENERIC) $(OBJS_SERIAL) $(OBJS_OMP)
DEPS         := $(OBJS:.o=.d)

# Final target
TARGET := $(BIN_DIR)/deliverable1_2025_2026
# Serial-specific target
TARGET_SERIAL := $(BIN_DIR)/serial_spmv
# OpenMP-specific target
TARGET_OMP := $(BIN_DIR)/openmp_spmv
# MPI target
TARGET_MPI := $(BIN_DIR)/mpi


TARGET_OMP_BINNING := $(BIN_DIR)/openmp_spmv_binning

TARGET_OMP_GUIDED := $(BIN_DIR)/openmp_spmv_guided
TARGET_OMP_DYNAMIC := $(BIN_DIR)/openmp_spmv_dynamic

.PHONY: all run clean datasets list-datasets clean-datasets serial-only openmp-static-only openmp-binning-only openmp-dynamic-only openmp-guided-only

.PHONY: all run clean datasets list-datasets clean-datasets serial-only openmp-static-only openmp-binning-only openmp-dynamic-only

all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

# Rule to compile only main_serial and SpMVSerial (plus generic objects)
serial-only: $(TARGET_SERIAL)

$(TARGET_SERIAL): $(OBJS_SERIAL_ONLY) $(OBJS_GENERIC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS_SERIAL_ONLY) $(OBJS_GENERIC) -o $@ $(LDFLAGS) $(LDLIBS)

# Rule to compile only main_openmp and SpMVOpenMP (plus generic objects)
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

openmp-guided-only: $(TARGET_OMP_GUIDED)

$(TARGET_OMP_GUIDED): $(OBJS_OMP_GUIDED_ONLY) $(OBJS_GENERIC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS_OMP_GUIDED_ONLY) $(OBJS_GENERIC) -o $@ $(LDFLAGS) $(LDLIBS)

mpi: $(TARGET_MPI)

$(TARGET_MPI): $(SRCS_MPI)
	@mkdir -p $(BIN_DIR)
	$(MPICC) $(SRCS_MPI) -o $@ -g

# Specific compilation rules
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

$(OBJ_DIR)/src/openMP/%.o: src/openMP/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_OMP) -MMD -MP -c $< -o $@

$(OBJ_DIR)/src/mpi/%.o: src/mpi/%.c
	@mkdir -p $(dir $@)
	$(MPICC) $(CXXFLAGS_MPI) -MMD -MP -c $< -o $@

# Run
run: $(TARGET)
	./$(TARGET)

# Clean
clean:
	$(RM) -r $(BIN_DIR) $(OBJ_DIR)

# Datasets download/extraction section
datasets: $(LIST_FILE)
	@mkdir -p $(DATASET_DIR)
	@set -e; \
	while IFS= read -r url || [ -n "$$url" ]; do \
	 [ -z "$$url" ] && continue; \
	 case "$$url" in \#*) continue;; esac; \
	 echo "Processing: $$url"; \
	 fname=$${url##*/}; \
	 archive_path="$(DATASET_DIR)/$$fname"; \
	 if command -v wget >/dev/null 2>&1; then \
	  echo "Downloading $$url"; \
	  wget -q -c -P "$(DATASET_DIR)" "$$url"; \
	 elif command -v curl >/dev/null 2>&1; then \
	  echo "Downloading $$url"; \
	  curl -L --fail --retry 3 -o "$$archive_path" "$$url"; \
	 else \
	  echo "Error: wget or curl required" >&2; exit 1; \
	 fi; \
	 case "$$fname" in \
	  *.tar.gz|*.tgz) \
	   tmpdir="$(DATASET_DIR)/.tmp_extract_$$PPID.$$RANDOM"; \
	   mkdir -p "$$tmpdir"; \
	   echo "Extracting $$fname into $$tmpdir"; \
	   tar -xzf "$$archive_path" -C "$$tmpdir"; \
	   mtx_count=$$(find "$$tmpdir" -type f -name '*.mtx' | wc -l | tr -d '[:space:]'); \
	   if [ "$$mtx_count" -eq 0 ]; then \
	    echo "Warning: no .mtx found in $$fname"; \
	   else \
	    echo "Found $$mtx_count .mtx files, moving into $(DATASET_DIR)"; \
	    find "$$tmpdir" -type f -name '*.mtx' -exec mv -t "$(DATASET_DIR)" {} +; \
	   fi; \
	   rm -rf "$$tmpdir"; \
	   rm -f "$$archive_path"; \
	   ;; \
	  *.tar) \
	   tmpdir="$(DATASET_DIR)/.tmp_extract_$$PPID.$$RANDOM"; \
	   mkdir -p "$$tmpdir"; \
	   echo "Extracting $$fname into $$tmpdir"; \
	   tar -xf "$$archive_path" -C "$$tmpdir"; \
	   mtx_count=$$(find "$$tmpdir" -type f -name '*.mtx' | wc -l | tr -d '[:space:]'); \
	   if [ "$$mtx_count" -eq 0 ]; then \
	    echo "Warning: no .mtx found in $$fname"; \
	   else \
	    echo "Found $$mtx_count .mtx files, moving into $(DATASET_DIR)"; \
	    find "$$tmpdir" -type f -name '*.mtx' -exec mv -t "$(DATASET_DIR)" {} +; \
	   fi; \
	   rm -rf "$$tmpdir"; \
	   rm -f "$$archive_path"; \
	   ;; \
	  *.mtx) \
	   echo ".mtx file: no extraction needed"; \
	   ;; \
	  *) \
	   echo "Unsupported format: $$fname"; \
	   rm -f "$$archive_path"; \
	   ;; \
	 esac; \
	done < "$(LIST_FILE)"
	@echo "Operation completed. Only .mtx files are available in $(DATASET_DIR)"

list-datasets: $(LIST_FILE)
	@awk '!/^[[:space:]]*(#|$$)/ {print}' "$(LIST_FILE)"

clean-datasets:
	@rm -rf "$(DATASET_DIR)"

# Include generated dependencies
-include $(DEPS)
